use std::ffi::c_void;
use std::fs::File;
use std::io::prelude::*;
use std::pin::{pin, Pin};
use windows::Win32::System::Console::AllocConsole;
use windows::Win32::System::Diagnostics::Debug::WriteProcessMemory;
use windows::Win32::System::Memory::{VirtualProtect, PAGE_PROTECTION_FLAGS};

use windows::{core::*, Win32::UI::WindowsAndMessaging::MessageBoxA};
use windows::{Win32::Foundation::*, Win32::System::SystemServices::*};

type Addr = u32;
type Offset = u32;

mod Addresses {
    use crate::{Addr, Offset};
    pub const D3D9Device: Addr = 0xD2A15C;
    pub const D3D9DeviceOffset: Offset = 0x3864;
}

mod Asm {
    pub const PUSHAD: u8 = 0x60;
    pub const POPAD: u8 = 0x61;
    pub const CALL: u8 = 0xE8;
    pub const RET: u8 = 0xC3;
    pub const INT3: u8 = 0xCC;
}

#[no_mangle]
pub unsafe extern "stdcall" fn EndScene_hook() {
    println!("XD");
}

struct InstructionBuffer<const N: usize> {
    instructions: [u8; N],
    current_offset: usize,
}

impl<const N: usize> InstructionBuffer<N> {
    fn new() -> Pin<Box<Self>> {
        Box::pin(Self {
            instructions: [Asm::INT3; N],
            current_offset: 0,
        })
    }
    fn get_address(&self) -> *const u8 {
        self.instructions.as_ptr()
    }

    fn generate_call_opcodes(&self, to: Addr) -> [u8; 5] {
        let target = to - (self.get_address() as u32 + N as u32) - 4;
        let mut array = [0; 5];
        array[0] = Asm::CALL;
        array[1..].copy_from_slice(&target.to_le_bytes());
        array
    }

    fn push_call_to(&mut self, addr: Addr) {
        let call = self.generate_call_opcodes(addr);
        self.push_slice(&call);
    }
    fn push_slice(&mut self, slice: &[u8]) {
        self.instructions[self.current_offset..].copy_from_slice(slice);
        self.current_offset += slice.len();
    }
    fn push(&mut self, op: u8) {
        self.instructions[self.current_offset] = op;
        self.current_offset += 1;
    }
}

struct Trampoline<const N: usize> {
    patch_addr: Addr,
    original_instructions: Box<[u8]>,
    instruction_buffer: Pin<Box<InstructionBuffer<N>>>,
}

impl<const N: usize> Trampoline<N> {
    fn enable(&self) -> Result<()> {
        const PAGE_EXECUTE_READWRITE: u32 = 0x40;
        let mut old_flags = PAGE_PROTECTION_FLAGS(0);
        unsafe {
            VirtualProtect(
                self.instruction_buffer.get_address() as *const c_void,
                N,
                PAGE_PROTECTION_FLAGS(PAGE_EXECUTE_READWRITE),
                &mut old_flags,
            )
        }
    }
    fn disable(&self) {
        todo!();
    }
}

fn deref<T: Copy, const N: u8>(addr: Addr) -> T {
    let mut t = addr;
    for _ in 1..N {
        t = unsafe { *(t as *const Addr) };
    }
    unsafe { *(t as *const T) }
}

fn copy_original_opcodes<const N: usize>(addr: Addr) -> Box<[u8; N]> {
    let mut destination = [0; N];
    unsafe {
        std::ptr::copy_nonoverlapping(
            addr as *const u8,
            destination.as_mut_ptr(),
            destination.len(),
        )
    };
    destination.into()
}

fn prepare_endscene_trampoline() -> Trampoline<7> {
    let EndScene = find_EndScene();
    let mut res = Trampoline::<7> {
        patch_addr: EndScene,
        original_instructions: copy_original_opcodes::<7>(EndScene),
        instruction_buffer: InstructionBuffer::<7>::new(),
    };
    res.instruction_buffer.push(Asm::PUSHAD);
    res.instruction_buffer.push_call_to(EndScene_hook as u32);
    res.instruction_buffer.push(Asm::POPAD);
    res
}

fn find_EndScene() -> Addr {
    // let wowd3d9 = unsafe { *(Addresses::D3D9Device as *const Addr) };
    let wowd3d9 = deref::<Addr, 1>(Addresses::D3D9Device);
    let d3d9 = deref::<Addr, 2>(wowd3d9 + Addresses::D3D9DeviceOffset);
    let res = d3d9 + 0x2A * 4;
    let mut f = File::create("C:\\Users\\elias\\pylly.txt").unwrap();
    writeln!(f, "{:x} {:x} {:x}", wowd3d9, d3d9, res).unwrap();
    return res;
}

#[no_mangle]
#[allow(non_snake_case, unused_variables)]
extern "system" fn DllMain(dll_module: HINSTANCE, call_reason: u32, _: *mut ()) -> bool {
    match call_reason {
        DLL_PROCESS_ATTACH => {
            let EndScene = find_EndScene();
        }
        // DLL_PROCESS_DETACH => ),
        _ => (),
    }

    true
}
