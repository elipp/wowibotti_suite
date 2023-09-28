use std::ffi::c_void;
use std::fs::File;
use std::fs::OpenOptions;
use std::io::prelude::*;
use std::pin::{pin, Pin};
use std::sync::RwLock;

use addresses::LUA_Prot_patchaddr;
use windows::Win32::Foundation::{GENERIC_READ, GENERIC_WRITE};
use windows::Win32::Storage::FileSystem::{
    CreateFileW, FILE_FLAGS_AND_ATTRIBUTES, FILE_GENERIC_WRITE, FILE_SHARE_WRITE, OPEN_EXISTING,
};
use windows::Win32::System::Console::SetStdHandle;
use windows::Win32::System::Console::{AllocConsole, GetStdHandle, STD_OUTPUT_HANDLE};
use windows::Win32::System::Diagnostics::Debug::WriteProcessMemory;
use windows::Win32::System::Memory::{VirtualProtect, PAGE_PROTECTION_FLAGS};
use windows::Win32::System::Threading::GetCurrentProcess;
use windows::{core::*, Win32::UI::WindowsAndMessaging::MessageBoxA};
use windows::{Win32::Foundation::*, Win32::System::SystemServices::*};

use lazy_static::lazy_static;

const INSTRBUF_SIZE: usize = 64;

lazy_static! {
    static ref ENABLED_PATCHES: RwLock<Vec<Patch>> = RwLock::new(vec![]);
    static ref NEED_INIT: RwLock<bool> = RwLock::new(true);
}

type Addr = u32;
type Offset = u32;

pub mod asm;
pub mod objectmanager;

use objectmanager::ObjectManager;

pub fn wide_null(s: &str) -> Vec<u16> {
    s.encode_utf16().chain(Some(0)).collect()
}

// this is pre-AllocConsole era :D
macro_rules! dump_to_logfile {
    ($fmt:literal, $($args:expr),*) => {{
        let mut file = OpenOptions::new()
            .write(true)
            .append(true)
            .open("C:\\Users\\elias\\log.txt")
            .unwrap();
        writeln!(file, $fmt, $($args),*).unwrap();
    }}
}

pub mod addresses;

fn reopen_stdout() -> HANDLE {
    match unsafe {
        CreateFileW(
            PCWSTR::from_raw(wide_null("CONOUT$").as_ptr()),
            (GENERIC_READ | GENERIC_WRITE).0,
            FILE_SHARE_WRITE,
            None,
            OPEN_EXISTING,
            FILE_FLAGS_AND_ATTRIBUTES(0),
            None,
        )
    } {
        Ok(handle) => handle,
        _ => INVALID_HANDLE_VALUE,
    }
}

fn main_entrypoint() {
    match ObjectManager::new() {
        Ok(om) => {
            for o in om {
                println!("{o}");
            }
        }
        _ => {}
    }
}

fn prepare_lua_prot_patch() -> Patch {
    let original_instructions = Vec::from_iter(copy_original_opcodes::<9>(LUA_Prot_patchaddr));

    let mut instruction_buffer = InstructionBuffer::new();
    instruction_buffer.push_slice(&[0xB8, 0x01, 0x00, 0x00, 0x00, 0x5D, 0xC3]);

    Patch {
        name: "Lua_Prot".to_string(),
        patch_addr: LUA_Prot_patchaddr,
        original_instructions,
        instruction_buffer,
        kind: PatchKind::OverWrite,
    }
}

#[no_mangle]
pub unsafe extern "stdcall" fn EndScene_hook() {
    if *NEED_INIT.read().expect("read lock for NEED_INIT") {
        AllocConsole().expect("AllocConsole");
        SetStdHandle(STD_OUTPUT_HANDLE, reopen_stdout()).expect("Reopen CONOUT$");

        *NEED_INIT.write().expect("write lock for NEED_INIT") = false;

        let lua_prot_patch = prepare_lua_prot_patch();
        lua_prot_patch.enable();
        let mut patches = ENABLED_PATCHES
            .write()
            .expect("write lock for ENABLED_PATCHES");

        patches.push(lua_prot_patch);

        println!("wowibottihookdll_rust: init done! :D enabled_patches:");
        for p in patches.iter() {
            println!("* {}", p.name);
        }
    } else {
        main_entrypoint();
    }
}

#[derive(Debug)]
struct InstructionBuffer {
    instructions: Pin<Box<[u8; INSTRBUF_SIZE]>>,
    current_offset: usize,
}

impl InstructionBuffer {
    fn new() -> Self {
        Self {
            instructions: Pin::new(Box::new([asm::INT3; INSTRBUF_SIZE])),
            current_offset: 0,
        }
    }

    fn len(&self) -> usize {
        self.current_offset
    }

    fn get_address(&self) -> *const u8 {
        self.instructions.as_ptr()
    }

    fn push_slice(&mut self, slice: &[u8]) {
        self.instructions[self.current_offset..self.current_offset + slice.len()]
            .copy_from_slice(&slice);
        self.current_offset += slice.len();
    }

    fn push_call_to(&mut self, to: Addr) {
        self.push(asm::CALL);
        let target = to - (self.get_address() as u32 + self.current_offset as u32) - 4;
        self.push_slice(&target.to_le_bytes());
    }

    fn push(&mut self, op: u8) {
        self.instructions[self.current_offset] = op;
        self.current_offset += 1;
    }

    fn push_default_return(&mut self, patch_addr: u32, patch_size: usize) {
        let target = patch_addr + patch_size as u32;
        self.push(asm::PUSH);
        self.push_slice(&target.to_le_bytes());
        self.push(asm::RET);
    }
}

#[derive(Debug)]
enum PatchKind {
    JmpToTrampoline,
    OverWrite,
}

#[derive(Debug)]
struct Patch {
    name: String,
    patch_addr: Addr,
    original_instructions: Vec<u8>,
    instruction_buffer: InstructionBuffer,
    kind: PatchKind,
}

#[derive(Debug)]
pub enum LoleError {
    PatchError(String),
    ClientConnectionIsNull,
    ObjectManagerIsNull,
}

impl Patch {
    unsafe fn commit(&self, patch: &InstructionBuffer) {
        let process_handle = GetCurrentProcess();
        WriteProcessMemory(
            process_handle,
            self.patch_addr as *const c_void,
            patch.get_address() as *const c_void,
            patch.len(),
            None,
        )
        .expect("WriteProcessMemory")
    }

    unsafe fn enable(&self) -> std::result::Result<(), LoleError> {
        match self.kind {
            PatchKind::JmpToTrampoline => {
                const PAGE_EXECUTE_READWRITE: u32 = 0x40;
                let mut old_flags = PAGE_PROTECTION_FLAGS(0);
                VirtualProtect(
                    self.instruction_buffer.get_address() as *const c_void,
                    self.instruction_buffer.len(),
                    PAGE_PROTECTION_FLAGS(PAGE_EXECUTE_READWRITE),
                    &mut old_flags,
                )
                .map_err(|e| LoleError::PatchError(format!("{:?}", e)))?;

                let mut patch = InstructionBuffer::new();
                patch.push(asm::JMP);
                let jmp_target =
                    self.instruction_buffer.get_address() as i32 - self.patch_addr as i32 - 5;
                patch.push_slice(&jmp_target.to_le_bytes());
                self.commit(&patch);
            }
            PatchKind::OverWrite => unsafe {
                self.commit(&self.instruction_buffer);
            },
        }
        Ok(())
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

fn copy_original_opcodes<const N: usize>(addr: Addr) -> [u8; N] {
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

fn prepare_endscene_trampoline() -> Patch {
    let EndScene = find_EndScene();
    let original_instructions = Vec::from_iter(copy_original_opcodes::<7>(EndScene));

    let mut instruction_buffer = InstructionBuffer::new();
    instruction_buffer.push(asm::PUSHAD);
    instruction_buffer.push_call_to(EndScene_hook as u32);
    instruction_buffer.push(asm::POPAD);
    instruction_buffer.push_slice(&original_instructions);
    instruction_buffer.push_default_return(EndScene, 7);

    Patch {
        name: "EndScene".to_string(),
        patch_addr: EndScene,
        original_instructions,
        instruction_buffer,
        kind: PatchKind::JmpToTrampoline,
    }
}

fn find_EndScene() -> Addr {
    let wowd3d9 = deref::<Addr, 1>(addresses::D3D9Device);
    let d3d9 = deref::<Addr, 2>(wowd3d9 + addresses::D3D9DeviceOffset);
    deref::<Addr, 1>(d3d9 + 0x2A * 4)
}

#[no_mangle]
#[allow(non_snake_case, unused_variables)]
extern "system" fn DllMain(dll_module: HINSTANCE, call_reason: u32, _: *mut ()) -> bool {
    match call_reason {
        DLL_PROCESS_ATTACH => {
            let tramp = prepare_endscene_trampoline();
            unsafe {
                tramp.enable();
            }
            ENABLED_PATCHES.write().unwrap().push(tramp);
        }
        // DLL_PROCESS_DETACH => ),
        _ => (),
    }

    true
}
