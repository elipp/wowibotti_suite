pub const PUSHAD: u8 = 0x60;
pub const PUSH_EAX: u8 = 0x50;
pub const PUSH_ECX: u8 = 0x51;
pub const PUSH_EDX: u8 = 0x52;
pub const PUSH_EBX: u8 = 0x53;
pub const PUSH_ESI: u8 = 0x56;
pub const PUSH_EDI: u8 = 0x57;
pub const POP_ECX: u8 = 0x59;
pub const POPAD: u8 = 0x61;
pub const CALL: u8 = 0xE8;
const _RETN: u8 = 0xC2;
pub const RET: u8 = 0xC3;
pub const INT3: u8 = 0xCC;
pub const PUSH_IMM: u8 = 0x68;
pub const JMP: u8 = 0xE9;
pub const NOP: u8 = 0x90;

#[allow(non_snake_case)]
pub fn RETN(n: u16) -> [u8; 3] {
    let bytes = n.to_le_bytes();
    [_RETN, bytes[0], bytes[1]]
}
