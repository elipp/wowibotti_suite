use std::arch::asm;

use windows::Win32::Networking::WinSock::SOCKET;
use windows::Win32::Networking::WinSock::{self, SEND_RECV_FLAGS};
use windows::Win32::System::SystemInformation::GetTickCount;

use crate::addrs::offsets::{self};
use crate::assembly;
use crate::opcodes::{is_movement_opcode, MSG_MOVE_SET_FACING, OPCODE_NAME_MAP};
use crate::patch::{copy_original_opcodes, write_addr, InstructionBuffer, Patch, PatchKind};
use crate::vec3::Vec3;
use crate::{objectmanager::GUID, patch::deref, Addr, LoleError, LoleResult};

#[derive(Debug)]
pub struct WowSocket(Addr, SOCKET);

impl WowSocket {}

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

#[cfg(feature = "tbc")]
impl WowSocket {
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
}
#[cfg(feature = "wotlk")]
impl WowSocket {
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

    pub fn send_packet(&self, mut packet: Vec<u8>) -> LoleResult<()> {
        self.encrypt_packet(&mut packet)?;
        let res = unsafe { WinSock::send(self.1, &packet, SEND_RECV_FLAGS(0)) };
        if res < 0 || res as usize != packet.len() {
            return Err(LoleError::SocketSendError(format!(
                "WinSock::send() returned {res}, should match packet len (= {})",
                packet.len()
            )));
        }
        Ok(())
    }
}

pub fn pack_guid(guid: GUID) -> Box<[u8]> {
    let mut guid = guid;
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
    // deref::<u32, 1>(TICK_COUNT) // + deref::<u32, 1>(UNK_CLOCK_DRIFT)
    // GetOsTickCount()
    unsafe { GetTickCount() }
}

pub mod movement_flags {
    pub const NOT_MOVING: u8 = 0x0;
    pub const FORWARD: u8 = 0x1;
    pub const BACKWARD: u8 = 0x2;
    pub const STRAFE_LEFT: u8 = 0x4;
    pub const STRAFE_RIGHT: u8 = 0x8;
    pub const IN_AIR: u8 = 0x10;
}

#[cfg(feature = "tbc")]
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
    wowsocket.send_packet(packet)?;
    Ok(())
}

// 0x0, 0x27, 0xDA, 0x0, 0x0, 0x0, 0x87, 0xF2, 0xAD, 0x6D,
// 0x6, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xA9, 0x6A, 0x11,
// 0x0, 0xA, 0x76, 0x9A, 0xC5, 0x2D, 0xDC, 0x93, 0xC4, 0x81,
// 0xDD, 0xFA, 0x43, 0x3C, 0x61, 0xF6, 0x3F, 0x0, 0x0, 0x0,
// 0x0,

// 0x00, 0x27, 0xDA, 0x00, 0x00, 0x00, 0x87, 0xCF, 0xD2, 0x6D,
// 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9D, 0x87, 0x2E,
// 0x00, 0x08, 0xF7, 0x09, 0xC6, 0x05, 0x64, 0x1D, 0x44, 0xA7,
// 0x2F, 0xBC, 0x42, 0xC9, 0x85, 0x1D, 0x40, 0x8B, 0x03, 0x00,
// 0x00,

#[cfg(feature = "wotlk")]
pub mod facing {
    use std::{arch::asm, cell::Cell};

    use super::movement_flags::NOT_MOVING;
    use super::{pack_guid, read_os_tick_count};
    use crate::addrs::offsets;
    use crate::ctm;
    use crate::objectmanager::WowObject;
    use crate::socket::get_wow_sockobj;
    use crate::LoleError;
    use crate::{
        chatframe_print,
        objectmanager::{ObjectManager, GUID},
        opcodes::MSG_MOVE_SET_FACING,
        vec3::{Vec3, TWO_PI},
        LoleResult,
    };

    const SETFACING_DELAY_MILLIS: u64 = 300;

    thread_local! {
        pub static SETFACING_STATE: Cell<(f32, std::time::Instant)> = Cell::new((0.0, std::time::Instant::now()));
        pub static FALL_TIME: Cell<u32> = Cell::new(0);
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
        chatframe_print!("called set_facing_local: {}", angle);
        Ok(())
    }

    pub fn create_default_facing_packet() -> LoleResult<Vec<u8>> {
        let om = ObjectManager::new()?;
        let player = om.get_player()?;
        let packet = create_facing_packet(
            player.get_guid(),
            player.get_pos(),
            player.get_rotvec().to_rot_value(),
            NOT_MOVING,
        );
        Ok(packet)
    }

    pub fn create_facing_packet(
        player_guid: GUID,
        pos: Vec3,
        angle: f32,
        movement_flags: u8,
    ) -> Vec<u8> {
        let mut packet = vec![0x0, 0x0]; // should set size later, since due to guid packing, the size isn't fixed
        packet.extend(MSG_MOVE_SET_FACING.to_le_bytes());
        packet.extend([0x0; 2]);
        let guid_packed = pack_guid(player_guid);
        packet.extend_from_slice(&guid_packed);

        // NOTE: IN_AIR belongs to the byte after movement flags, if that's ever needed :D
        packet.extend([movement_flags, 0x0, 0x0, 0x0, 0x0, 0x0]);

        let ticks = read_os_tick_count();
        packet.extend(ticks.to_le_bytes());

        packet.extend(
            [pos.x, pos.y, pos.z, angle]
                .into_iter()
                .flat_map(|f| f.to_le_bytes()),
        );

        packet.extend(FALL_TIME.get().to_le_bytes());

        let packet_len = (packet.len() - 2) as u16;
        packet[..2].copy_from_slice(&packet_len.to_be_bytes());
        packet
    }

    pub fn set_facing_remote(
        player_guid: GUID,
        pos: Vec3,
        angle: f32,
        movement_flags: u8,
    ) -> LoleResult<()> {
        let packet = create_facing_packet(player_guid, pos, angle, movement_flags);
        let wowsocket = get_wow_sockobj()?;
        wowsocket.send_packet(packet)?;
        chatframe_print!("called set_facing_remote: {}", angle);
        Ok(())
    }

    pub fn set_facing(player: WowObject, angle: f32, movement_flags: u8) -> LoleResult<()> {
        let angle = angle.rem_euclid(TWO_PI);
        let (prev_angle, timestamp) = SETFACING_STATE.get();
        let timeout_passed =
            timestamp.elapsed() > std::time::Duration::from_millis(SETFACING_DELAY_MILLIS);

        if timeout_passed {
            if !ctm::event_in_progress()? {
                set_facing_remote(player.get_guid(), player.get_pos(), angle, movement_flags)?;
                set_facing_local(angle)?;
                SETFACING_STATE.set((angle, std::time::Instant::now()));
            } else {
                chatframe_print!("ctm event in progress; not setting facing");
            }
        }
        Ok(())
    }

    pub fn get_facing_angle_to_target() -> LoleResult<Option<f32>> {
        let om = ObjectManager::new()?;
        let (player, target) = om.get_player_and_target()?;
        if let Some(target) = target {
            let (pp, tp) = (player.get_pos(), target.get_pos());
            let tpdiff_unit = (tp - pp).zero_z().unit();
            Ok(Some(tpdiff_unit.to_rot_value()))
        } else {
            Ok(None)
        }
    }
}

fn read_cast_count() -> LoleResult<u8> {
    let byte = deref::<u32, 1>(offsets::SPELL_CAST_COUNTER);
    let [_, counter, _, _] = byte.to_le_bytes();
    Ok(counter)
}

fn increment_cast_count() -> LoleResult<()> {
    let byte = deref::<u32, 1>(offsets::SPELL_CAST_COUNTER);
    let [a, b, c, d] = byte.to_le_bytes();
    let new_value = u32::from_le_bytes([a, b.wrapping_add(1), c, d]);
    write_addr(offsets::SPELL_CAST_COUNTER, &[new_value])
}

pub fn cast_gtaoe(spellid: i32, pos: Vec3) -> LoleResult<()> {
    let mut packet: Vec<u8> = vec![0x0, 0x1B, 0x2E, 0x1, 0x0, 0x0]; // packet size, 0x012E -> CMSG_CAST_SPELL
    packet.push(read_cast_count()?);
    packet.extend(spellid.to_le_bytes());
    packet.extend([0x0, 0x40, 0x0, 0x0, 0x0, 0x0]); // flags + something
    packet.extend(pos.x.to_le_bytes());
    packet.extend(pos.y.to_le_bytes());
    packet.extend(pos.z.to_le_bytes());
    let wowsocket = get_wow_sockobj()?;
    wowsocket.send_packet(packet)?;
    increment_cast_count()?;
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
            if opcode == MSG_MOVE_SET_FACING {
                // just to get the length
                let dummy = facing::create_default_facing_packet().unwrap();

                // MSG_MOVE_SET_FACING packets can have length of 57 (with falling spline data, some trigonometric stuff etc)
                if dummy.len() == packet_bytes.len() {
                    let mut buf = [0u8; 4];
                    buf.copy_from_slice(&packet_bytes[(dummy.len() - 4)..]);
                    facing::FALL_TIME.set(u32::from_le_bytes(buf));
                }
            }
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
                packet_bytes[17],
                packet_bytes[18],
                packet_bytes[19],
                packet_bytes[20],
            ]);
        }
    } else {
        println!("warning: unknown opcode 0x{:X}", opcode);
    }
    // print_as_c_array("outbound packet", packet_bytes);
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
