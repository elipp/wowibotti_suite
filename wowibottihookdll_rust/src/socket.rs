use std::arch::asm;

use windows::Win32::Networking::WinSock::SOCKET;
use windows::Win32::Networking::WinSock::{self, SEND_RECV_FLAGS};
use windows::Win32::System::SystemInformation::GetTickCount;

use crate::objectmanager::ObjectManager;
use crate::vec3::Vec3;
use crate::{objectmanager::GUID, patch::deref, Addr, LoleError, LoleResult};

mod socket {
    use crate::{Addr, Offset};
    pub const CONNECTION: Addr = 0xD4332C;
    pub const SOCKOBJ: Offset = 0x2198;
}

#[derive(Debug)]
pub struct WowSocket(Addr, SOCKET);

pub fn get_wow_sockobj() -> LoleResult<WowSocket> {
    let tmp = deref::<Addr, 1>(socket::CONNECTION);
    if tmp == 0 {
        return Err(LoleError::WowSocketNotAvailable);
    }
    let sockobj = deref::<Addr, 1>(tmp + socket::SOCKOBJ);
    if sockobj == 0 {
        return Err(LoleError::WowSocketNotAvailable);
    }
    let socket = deref::<usize, 1>(sockobj + 0x4);
    if socket == 0 {
        return Err(LoleError::WowSocketNotAvailable);
    }
    Ok(WowSocket(sockobj, SOCKET(socket)))
}

const ENCRYPT_BYTE: Addr = 0x41F610;

impl WowSocket {
    pub fn encrypt_packet(&self, packet: &mut [u8]) -> LoleResult<()> {
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
                    encrypt_byte = in(reg) ENCRYPT_BYTE,
                    out("edx") _,
                    out("eax") _,
                }
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

const SetFacing: Addr = 0x7B9DE0;

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
            func_addr = in(reg) SetFacing,
            out("eax") _, // SetFacing clobbers "eax"
            out("edx") _,
        }
    }
    Ok(())
}

const MSG_SET_FACING: u16 = 0x00DA;

pub fn set_facing_remote(pos: Vec3, angle: f32) -> LoleResult<()> {
    let mut packet = vec![0x00, 0x21]; // size "without" header
    packet.extend_from_slice(&MSG_SET_FACING.to_le_bytes());
    packet.extend_from_slice(&[0x0; 7]);
    let ticks = unsafe { GetTickCount() };
    packet.extend_from_slice(&ticks.to_le_bytes());

    let xyzr: Vec<_> = [pos.x, pos.y, pos.z, angle]
        .into_iter()
        .flat_map(|f| f.to_le_bytes())
        .collect();

    packet.extend_from_slice(&xyzr);

    packet.extend_from_slice(&[0x0; 4]);

    if packet.len() != 0x23 {
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
            "WinSock::send() returned {res}, while packet len = {}",
            packet.len()
        )));
    }
    Ok(())
}
