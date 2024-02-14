use std::arch::asm;

use windows::Win32::Networking::WinSock::SOCKET;
use windows::Win32::Networking::WinSock::{self, SEND_RECV_FLAGS};

use crate::addrs::offsets::{self, TAINT_CALLER, TICK_COUNT, UNK_CLOCK_DRIFT};
use crate::lua::{RUN_SCRIPT_AFTER_N_FRAMES, SETFACING_STATE};
use crate::objectmanager::ObjectManager;
use crate::opcodes::{is_movement_opcode, MSG_MOVE_SET_FACING, OPCODE_NAME_MAP};
use crate::patch::{
    copy_original_opcodes, read_elems_from_addr, write_addr, InstructionBuffer, Patch, PatchKind,
};
use crate::vec3::{Vec3, TWO_PI};
use crate::{assembly, chatframe_print, dostring, print_as_c_array, Offset};
use crate::{objectmanager::GUID, patch::deref, Addr, LoleError, LoleResult};

#[derive(Debug)]
pub struct WowSocket(Addr, SOCKET);

pub fn get_wow_sockobj() -> LoleResult<WowSocket> {
    let tmp = deref::<Addr, 1>(offsets::socket::CONNECTION);
    if tmp == 0 {
        return Err(LoleError::WowSocketNotAvailable);
    }
    let sockobj = deref::<Addr, 1>(tmp + offsets::socket::SOCKOBJ);
    if sockobj == 0 {
        return Err(LoleError::WowSocketNotAvailable);
    }
    let socket = deref::<usize, 1>(sockobj + 0x4);
    if socket == 0 {
        return Err(LoleError::WowSocketNotAvailable);
    }
    Ok(WowSocket(sockobj, SOCKET(socket)))
}

impl WowSocket {
    #[cfg(feature = "tbc")]
    pub fn encrypt_packet(&self, packet: &mut [u8]) -> LoleResult<()> {
        use crate::addrs::offsets::wow_cfuncs::SARC4_ENCRYPT_BYTE;
        if packet.len() < 6 {
            return Err(LoleError::PacketSynthError(format!(
                "packet too short ({} bytes)",
                packet.len()
            )));
        }
        unsafe {
            for i in 0..6 {
                let byte_addr = packet.as_mut_ptr().add(i);
                asm! {
                    "push {byte_addr}",
                    "call {encrypt_byte:e}",
                    in("ecx") self.0,
                    byte_addr = in(reg) byte_addr,
                    encrypt_byte = in(reg) SARC4_ENCRYPT_BYTE,
                    out("eax") _,
                    out("edx") _,
                }
            }
        }
        Ok(())
    }
    #[cfg(feature = "wotlk")]
    pub fn encrypt_packet(&self, packet: &mut [u8]) -> LoleResult<()> {
        use crate::addrs::offsets::wow_cfuncs::EncryptPacketHeader;
        if packet.len() < 6 {
            return Err(LoleError::PacketSynthError(format!(
                "packet too short ({} bytes)",
                packet.len()
            )));
        }
        unsafe {
            asm! {
                "push 0x6",
                "push {byte_addr:e}",
                "call {sarc4_encrypt:e}",
                in("ecx") self.0,
                byte_addr = in(reg) packet.as_mut_ptr(),
                sarc4_encrypt = in(reg) EncryptPacketHeader,
                out("eax") _,
                out("edx") _,
            }
        }
        Ok(())
    }
}

pub fn pack_guid(mut guid: GUID) -> Box<[u8]> {
    let mut packed = [0u8; 8 + 1];
    let mut size = 1;
    let mut i: u8 = 0;
    while guid != 0 {
        if guid & 0xFF != 0 {
            packed[0] |= 1 << i;
            packed[size] = (guid & 0xFF) as u8;
            size += 1;
        }
        guid >>= 8;
        i += 1;
    }
    packed[..size].into()
}

pub fn set_facing_local(angle: f32) -> LoleResult<()> {
    let om = ObjectManager::new()?;
    let player = om.get_player()?;
    let movement_info = player.get_movement_info();
    unsafe {
        asm! {
            "push {angle:e}",
            "call {func_addr:e}",
            in("ecx") movement_info,
            angle = in(reg) angle,
            func_addr = in(reg) offsets::wow_cfuncs::SetFacing,
            out("eax") _,
            out("edx") _,
        }
    }
    Ok(())
}

// opcode: MSG_MOVE_SET_FACING
// outbound packet:
// 0x0, 0x31, 0xDA, 0x0,
// 0x0, 0x0, 0x1, 0x10, 0x0, 0x0, 0x0,
// 0xA, 0x26, 0x28, 0x1, 0x3B, 0x97, 0xD0, 0x44, 0xB1,
// 0x31, 0x88, 0xC5, 0x43, 0xB2, 0xEB, 0x41, 0xD8, 0x89, 0x91,
// 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1A,
// 0x88, 0x23, 0xBE, 0xE5, 0xB6, 0x7C, 0xBF, 0x0, 0x0, 0xE0,
// 0x40,

pub fn read_os_tick_count() -> u32 {
    deref::<u32, 1>(TICK_COUNT) // + deref::<u32, 1>(UNK_CLOCK_DRIFT)
                                // GetOsTickCount()
}

pub mod movement_flags {
    pub const NOT_MOVING: u8 = 0x0;
    pub const FORWARD: u8 = 0x1;
    pub const BACKWARD: u8 = 0x2;
    pub const STRAFE_LEFT: u8 = 0x4;
    pub const STRAFE_RIGHT: u8 = 0x8;
    pub const IN_AIR: u8 = 0x10;
}

fn set_facing_remote(pos: Vec3, angle: f32, movement_flags: u8) -> LoleResult<()> {
    let mut packet = vec![0x00, 0x21]; // size "without" header
    packet.extend(MSG_MOVE_SET_FACING.to_le_bytes());

    packet.extend([0x0; 2]);
    packet.push(movement_flags);
    // NOTE: IN_AIR belongs to the byte after movement flags, if that's ever needed :D
    packet.extend([0x0; 4]);

    let ticks = read_os_tick_count() + 1000; // empirically, seems to be a drift of about 1000 (maybe all the movement business reads ticks from another source?)
    packet.extend(ticks.to_le_bytes());

    packet.extend(
        [pos.x, pos.y, pos.z, angle]
            .into_iter()
            .flat_map(|f| f.to_le_bytes()),
    );

    packet.extend([0x0; 4]);

    if packet.len() != 35 {
        println!("packet.len(): {}\npacket: {packet:x?}", packet.len());
        return Err(LoleError::PacketSynthError(format!(
            "expected packet of size 35, got {}",
            packet.len()
        )));
    }

    let wowsocket = get_wow_sockobj()?;
    wowsocket.encrypt_packet(&mut packet)?;

    let res = unsafe { WinSock::send(wowsocket.1, &packet, SEND_RECV_FLAGS(0)) };
    if res < 0 || res as usize != packet.len() {
        return Err(LoleError::SocketSendError(format!(
            "WinSock::send() returned {res}, should match packet len (= {})",
            packet.len()
        )));
    }
    Ok(())
}

pub fn set_facing(angle: f32, movement_flags: u8) -> LoleResult<()> {
    let om = ObjectManager::new()?;
    let player = om.get_player()?;
    let pos = player.get_pos();
    let angle = angle.rem_euclid(TWO_PI);
    chatframe_print!("calling set_facing: {}", angle);
    // set_facing_remote(pos, angle, movement_flags)?;
    set_facing_local(angle)?;
    dostring!("TurnLeftStart()")?; // this is to update does this send the new facing info to the server?
                                   // offsets::CUpdatePlayer();

    SETFACING_STATE.set((angle, std::time::Instant::now()));
    RUN_SCRIPT_AFTER_N_FRAMES.set(Some(("TurnLeftStop()", 2)));
    Ok(())
}

unsafe extern "stdcall" fn dump_outbound_packet(edi: Addr) {
    if edi == 0 {
        return println!("dump_outbound_packet: edi was null");
    }
    let packet_addr: Addr = deref::<_, 1>(edi + 0x8);
    let packet_size_total: Addr = deref::<_, 1>(edi + 0xC);
    if packet_addr == 0 || packet_size_total == 0 {
        return println!("dump_outbound_packet: invalid packet data");
    }
    let packet_bytes = std::slice::from_raw_parts(packet_addr as *const u8, packet_size_total);
    let opcode = u16::from_le_bytes([packet_bytes[2], packet_bytes[3]]);
    if let Some(opcode_name) = OPCODE_NAME_MAP.get(&opcode) {
        println!(
            "opcode: {}, packet_size_total: {}",
            opcode_name, packet_size_total
        );
        if is_movement_opcode(opcode) {
            let our_ticks = read_os_tick_count();
            #[cfg(feature = "tbc")]
            let ticks = u32::from_le_bytes([
                packet_bytes[11],
                packet_bytes[12],
                packet_bytes[13],
                packet_bytes[14],
            ]);
            #[cfg(feature = "wotlk")]
            let ticks = u32::from_le_bytes([
                packet_bytes[14],
                packet_bytes[15],
                packet_bytes[16],
                packet_bytes[17],
            ]);
            println!(
                "packet ticks: 0x{:08X}, our ticks: 0x{:08X}, diff = {}",
                ticks,
                our_ticks,
                ticks as i64 - our_ticks as i64
            );
        }
    } else {
        println!("warning: unknown opcode 0x{:X}", opcode);
    }
    print_as_c_array("outbound packet", packet_bytes);
}

#[cfg(feature = "tbc")]
pub fn prepare_dump_outbound_packet_patch() -> Patch {
    const DUMP_OUTBOUND_PACKET_PATCHADDR: Addr = 0x42050E;
    let patch_addr = DUMP_OUTBOUND_PACKET_PATCHADDR;
    let original_opcodes = copy_original_opcodes(patch_addr, 9);

    let mut patch_opcodes = InstructionBuffer::new();
    patch_opcodes.push(assembly::PUSHAD);
    patch_opcodes.push(assembly::PUSH_EDI);
    patch_opcodes.push_call_to(dump_outbound_packet as Addr);
    patch_opcodes.push(assembly::POPAD);
    patch_opcodes.push_slice(&original_opcodes);
    patch_opcodes.push(assembly::PUSH_IMM);
    patch_opcodes.push_slice(&(patch_addr + original_opcodes.len() as Offset).to_le_bytes());
    patch_opcodes.push(assembly::RET);

    Patch {
        name: "dump_outbound_packet_patch",
        patch_addr,
        patch_opcodes,
        original_opcodes,
        kind: PatchKind::JmpToTrampoline,
    }
}

#[cfg(feature = "wotlk")]
pub fn prepare_dump_outbound_packet_patch() -> Patch {
    use crate::addrs::offsets::wow_cfuncs::EncryptPacketHeader;

    const DUMP_OUTBOUND_PACKET_PATCHADDR: Addr = 0x46776E;
    let patch_addr = DUMP_OUTBOUND_PACKET_PATCHADDR;
    let original_opcodes = copy_original_opcodes(patch_addr, 5);

    let mut patch_opcodes = InstructionBuffer::new();
    patch_opcodes.push(assembly::PUSHAD);
    patch_opcodes.push(assembly::PUSH_EDI);
    patch_opcodes.push_call_to(dump_outbound_packet as Addr);
    patch_opcodes.push(assembly::POPAD);
    patch_opcodes.push_call_to(EncryptPacketHeader);
    patch_opcodes.push_default_return(DUMP_OUTBOUND_PACKET_PATCHADDR, 5);

    Patch {
        name: "dump_outbound_packet_patch",
        patch_addr,
        patch_opcodes,
        original_opcodes,
        kind: PatchKind::JmpToTrampoline,
    }
}
// opcode: MSG_MOVE_SET_FACING, our ticks: 0x13DEC1F
// outbound packet:
// 0x0, 0x21, 0xDA, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0,
// 0x0, 0x36, 0xEC, 0x3D, 0x1, 0x56, 0x87, 0xCF, 0x44, 0xFF,
// 0xB6, 0x88, 0xC5, 0xC9, 0x7F, 0xC0, 0x41, 0xE6, 0xEB, 0xB8,
// // 0x3F, 0x9, 0x0, 0x0, 0x0,

// opcode: MSG_MOVE_STOP, our ticks: 0x13F3335
// outbound packet:
// 0x0, 0x21, 0xB7, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
// 0x0, 0x4E, 0x33, 0x3F, 0x1, 0x60, 0x80, 0xCF, 0x44, 0x7C,
// 0x3E, 0x88, 0xC5, 0xF4, 0x93, 0xD3, 0x41, 0xD2, 0xE7, 0xB5,
// 0x3F, 0x9, 0x0, 0x0, 0x0,
