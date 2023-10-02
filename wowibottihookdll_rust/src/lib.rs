use std::mem::size_of;
use std::sync::{Mutex, MutexGuard, PoisonError};
use std::time::Duration;

use lua::{register_lop_exec_if_not_registered, unregister_lop_exec, Opcode};
use patch::prepare_endscene_trampoline;
use windows::core::PCWSTR;
use windows::Win32::Foundation::{GENERIC_READ, GENERIC_WRITE};
use windows::Win32::Storage::FileSystem::{
    CreateFileW, FILE_FLAGS_AND_ATTRIBUTES, FILE_SHARE_WRITE, OPEN_EXISTING,
};
use windows::Win32::System::Console::{AllocConsole, FreeConsole, STD_OUTPUT_HANDLE};
use windows::Win32::System::Console::{GetStdHandle, SetStdHandle};
use windows::Win32::System::LibraryLoader::FreeLibraryAndExitThread;
use windows::{Win32::Foundation::*, Win32::System::SystemServices::*};

use lazy_static::lazy_static;

lazy_static! {
    pub static ref ENABLED_PATCHES: Mutex<Vec<Patch>> = Mutex::new(vec![]);
    pub static ref NEED_INIT: Mutex<bool> = Mutex::new(true);
    pub static ref SHOULD_EJECT: Mutex<bool> = Mutex::new(false);
    pub static ref DLL_HANDLE: Mutex<HINSTANCE> = Mutex::new(HINSTANCE(0));
    pub static ref CONSOLE_CONOUT: Mutex<HANDLE> = Mutex::new(HANDLE(0));
    pub static ref ORIGINAL_STDOUT: Mutex<HANDLE> = Mutex::new(HANDLE(0));
}

type Addr = u32;
type Offset = u32;

pub mod addresses;
pub mod asm;
pub mod ctm;
pub mod lua;
pub mod objectmanager;
pub mod patch;
pub mod socket;
pub mod vec3;

use lua::register_lop_exec;

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

fn reopen_stdout() -> windows::core::Result<HANDLE> {
    unsafe {
        CreateFileW(
            PCWSTR::from_raw(wide_null("CONOUT$").as_ptr()),
            (GENERIC_READ | GENERIC_WRITE).0,
            FILE_SHARE_WRITE,
            None,
            OPEN_EXISTING,
            FILE_FLAGS_AND_ATTRIBUTES(0),
            None,
        )
    }
}

unsafe fn restore_original_stdout() -> windows::core::Result<()> {
    SetStdHandle(
        STD_OUTPUT_HANDLE,
        *ORIGINAL_STDOUT.lock().expect("ORIGINAL_STDOUT"),
    )
}

// impl From<

fn eject_dll() -> LoleResult<()> {
    let mut patches = ENABLED_PATCHES.lock().expect("ENABLED_PATCHES");
    for patch in patches.drain(..) {
        patch.disable().expect("disable patch");
    }
    unregister_lop_exec()?;
    unsafe {
        restore_original_stdout().expect("original stdout");
        CloseHandle(*CONSOLE_CONOUT.lock().unwrap()).unwrap();
        FreeConsole().expect("FreeConsole");
    }
    std::thread::spawn(|| unsafe {
        FreeLibraryAndExitThread(DLL_HANDLE.lock().expect("DLL_HANDLE").clone(), 0);
    });
    Ok(())
}

fn main_entrypoint() -> LoleResult<()> {
    if *SHOULD_EJECT.lock().expect("SHOULD_EJECT") {
        eject_dll()?;
    } else {
        register_lop_exec_if_not_registered()?;
        ctm::poll()?;
    }
    Ok(())
}

#[no_mangle]
pub unsafe extern "cdecl" fn EndScene_hook() {
    let mut need_init = NEED_INIT.lock().expect("NEED_INIT mutex");
    if *need_init {
        AllocConsole().expect("AllocConsole");

        let original_stdout = GetStdHandle(STD_OUTPUT_HANDLE).expect("stdout");
        *ORIGINAL_STDOUT.lock().expect("ORIGINAL_STDOUT") = original_stdout;

        let conout = reopen_stdout().expect("CONOUT$");
        *CONSOLE_CONOUT.lock().unwrap() = conout;

        SetStdHandle(STD_OUTPUT_HANDLE, conout).expect("Reopen CONOUT$");

        let mut patches = ENABLED_PATCHES
            .lock()
            .expect("write lock for ENABLED_PATCHES");

        let lua_prot = prepare_lua_prot_patch();
        lua_prot.enable().expect("lua_prot");
        patches.push(lua_prot);

        let ctm_finished = prepare_ctm_finished_patch();
        ctm_finished.enable().expect("ctm_finished");
        patches.push(ctm_finished);

        register_lop_exec().expect("lop_exec");
        println!("wowibottihookdll_rust: init done! :D enabled_patches:");
        for p in patches.iter() {
            println!("* {} @ 0x{:08X}", p.name, p.patch_addr);
        }

        *need_init = false;
    } else {
        if let Err(e) = main_entrypoint() {
            println!("Main entrypoint failed with {e:?}");
        }
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
    WowSocketNotAvailable,
    PacketSynthError(String),
    SocketSendError(String),
    MutexLockError,
    NotImplemented,
}

pub type LoleResult<T> = std::result::Result<T, LoleError>;

impl<T> From<PoisonError<T>> for LoleError {
    fn from(e: PoisonError<T>) -> Self {
        LoleError::MutexLockError
    }
}

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
            *DLL_HANDLE.lock().unwrap() = dll_module;
        }
        // DLL_PROCESS_DETACH => ),
        _ => (),
    }

    true
}
