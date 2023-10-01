use std::mem::size_of;
use std::sync::Mutex;

use lua::{register_lop_exec_if_not_registered, Opcode};
use patch::{prepare_endscene_trampoline, InstructionBuffer, PatchKind};
use windows::core::PCWSTR;
use windows::Win32::Foundation::{GENERIC_READ, GENERIC_WRITE};
use windows::Win32::Storage::FileSystem::{
    CreateFileW, FILE_FLAGS_AND_ATTRIBUTES, FILE_SHARE_WRITE, OPEN_EXISTING,
};
use windows::Win32::System::Console::SetStdHandle;
use windows::Win32::System::Console::{AllocConsole, STD_OUTPUT_HANDLE};
use windows::Win32::System::Diagnostics::Debug::WriteProcessMemory;
use windows::Win32::System::Threading::GetCurrentProcess;
use windows::{Win32::Foundation::*, Win32::System::SystemServices::*};

use lazy_static::lazy_static;

lazy_static! {
    pub static ref ENABLED_PATCHES: Mutex<Vec<Patch>> = Mutex::new(vec![]);
    pub static ref NEED_INIT: Mutex<bool> = Mutex::new(true);
}

type Addr = u32;
type Offset = u32;

pub mod addresses;
pub mod asm;
pub mod ctm;
pub mod lua;
pub mod objectmanager;
pub mod patch;
pub mod vec3;

use lua::register_lop_exec;
use objectmanager::ObjectManager;

use crate::ctm::prepare_ctm_finished_patch;
use crate::lua::prepare_lua_prot_patch;
use crate::patch::Patch;

pub fn wide_null(s: &str) -> Vec<u16> {
    s.encode_utf16().chain(Some(0)).collect()
}

// this is pre-AllocConsole era :D
#[macro_export]
macro_rules! dump_to_logfile {
    ($fmt:literal, $($args:expr),*) => {{
        use std::fs::OpenOptions;
        use std::io::prelude::*;
        let mut file = OpenOptions::new()
            .write(true)
            .append(true)
            .create(true)
            .open("C:\\Users\\elias\\log.txt")
            .unwrap();
        writeln!(file, $fmt, $($args),*).unwrap();
    }}
}

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
    register_lop_exec_if_not_registered();
}

#[no_mangle]
pub unsafe extern "cdecl" fn EndScene_hook() {
    let mut need_init = NEED_INIT.lock().expect("NEED_INIT mutex");
    if *need_init {
        AllocConsole().expect("AllocConsole");
        SetStdHandle(STD_OUTPUT_HANDLE, reopen_stdout()).expect("Reopen CONOUT$");

        let mut patches = ENABLED_PATCHES
            .lock()
            .expect("write lock for ENABLED_PATCHES");

        let lua_prot = prepare_lua_prot_patch();
        lua_prot.enable().expect("lua_prot");
        patches.push(lua_prot);

        let ctm_finished = prepare_ctm_finished_patch();
        ctm_finished.enable().expect("ctm_finished");
        patches.push(ctm_finished);

        println!("wowibottihookdll_rust: init done! :D enabled_patches:");
        for p in patches.iter() {
            println!("* {}@{:08X}", p.name, p.patch_addr);
        }
        register_lop_exec().expect("lop_exec");

        *need_init = false;
    } else {
        main_entrypoint();
    }
}

#[derive(Debug)]
pub enum LoleError {
    PatchError(String),
    ClientConnectionIsNull,
    ObjectManagerIsNull,
    PlayerNotFound,
    InvalidParam(String),
    MemoryWriteError,
    PartialMemoryWriteError,
    LuaStateIsNull,
    MissingOpcode,
    UnknownOpcode(i32),
    NullPointerError,
    InvalidRawString(String),
    InvalidEnumValue(String),
    InvalidOrUnimplementedOpcodeCall(Opcode, i32),
    NotImplemented,
}

pub type LoleResult<T> = std::result::Result<T, LoleError>;

#[no_mangle]
#[allow(non_snake_case, unused_variables)]
extern "system" fn DllMain(dll_module: HINSTANCE, call_reason: u32, _: *mut ()) -> bool {
    match call_reason {
        DLL_PROCESS_ATTACH => {
            let tramp = prepare_endscene_trampoline();
            unsafe {
                tramp.enable().expect("EndScene patch");
            }
            ENABLED_PATCHES
                .lock()
                .expect("ENABLED_PATCHES mutex")
                .push(tramp);
        }
        // DLL_PROCESS_DETACH => ),
        _ => (),
    }

    true
}
