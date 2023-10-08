use std::sync::Mutex;
use std::time::{Duration, Instant};
use windows::Win32::System::Threading::ExitProcess;
use windows::Win32::UI::WindowsAndMessaging::{MessageBoxW, MB_ICONERROR, MB_OK};

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
    pub static ref LAST_FRAME_TIME: Mutex<std::time::Instant> =
        Mutex::new(std::time::Instant::now());
}

// thread_local! { RefCell could do the trick! }

type Addr = usize;
type Offset = usize;

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

macro_rules! global_var {
    ($name:ident) => {
        match $name.lock() {
            Ok(lock) => lock,
            Err(e) => unsafe { fatal_error_exit(e.into()) },
        }
    };
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

macro_rules! windows_string {
    ($s:expr) => {
        PCWSTR::from_raw(wide_null($s).as_ptr())
    };
}

fn reopen_stdout() -> windows::core::Result<HANDLE> {
    unsafe {
        CreateFileW(
            windows_string!("CONOUT$"),
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
    SetStdHandle(STD_OUTPUT_HANDLE, *global_var!(ORIGINAL_STDOUT))
}

unsafe fn eject_dll() -> LoleResult<()> {
    let mut patches = global_var!(ENABLED_PATCHES);
    for patch in patches.drain(..) {
        patch.disable()?;
    }
    unregister_lop_exec()?;
    restore_original_stdout()?;
    CloseHandle(*global_var!(CONSOLE_CONOUT))?;
    FreeConsole()?;
    std::thread::spawn(|| {
        FreeLibraryAndExitThread(*global_var!(DLL_HANDLE), 0);
    });
    Ok(())
}

const ONE_SIXTIETH: Duration = Duration::from_micros((950000.0 / 60.0) as u64);

fn main_entrypoint() -> LoleResult<()> {
    let mut last_frame_time = global_var!(LAST_FRAME_TIME);
    register_lop_exec_if_not_registered()?;
    ctm::poll()?;
    if let Some(dt) = ONE_SIXTIETH.checked_sub(last_frame_time.elapsed()) {
        std::thread::sleep(dt);
    }
    *last_frame_time = Instant::now();
    Ok(())
}

unsafe fn initialize_dll() -> LoleResult<()> {
    AllocConsole()?;

    let original_stdout = GetStdHandle(STD_OUTPUT_HANDLE)?;
    *global_var!(ORIGINAL_STDOUT) = original_stdout;

    let conout = reopen_stdout()?;
    *global_var!(CONSOLE_CONOUT) = conout;

    SetStdHandle(STD_OUTPUT_HANDLE, conout)?;

    let mut patches = global_var!(ENABLED_PATCHES);

    let lua_prot = prepare_lua_prot_patch();
    lua_prot.enable()?;
    patches.push(lua_prot);

    let ctm_finished = prepare_ctm_finished_patch();
    ctm_finished.enable()?;
    patches.push(ctm_finished);

    register_lop_exec()?;

    println!("wowibottihookdll_rust: init done! :D enabled_patches:");

    for p in patches.iter() {
        println!("* {} @ 0x{:08X}", p.name, p.patch_addr);
    }

    Ok(())
}

unsafe fn fatal_error_exit(err: LoleError) -> ! {
    MessageBoxW(
        HWND(0),
        windows_string!(&format!("{err:?}")),
        windows_string!("wowibottihookdll_rust error :("),
        MB_OK | MB_ICONERROR, // MB_OK apparently waits for the user to click OK
    );
    ExitProcess(1);
}

#[no_mangle]
pub unsafe extern "cdecl" fn EndScene_hook() {
    let mut need_init = global_var!(NEED_INIT);
    if *need_init {
        if let Err(e) = initialize_dll() {
            fatal_error_exit(e);
        }
        *need_init = false;
    } else if *global_var!(SHOULD_EJECT) {
        if let Err(e) = eject_dll() {
            fatal_error_exit(e);
        }
    } else {
        match main_entrypoint() {
            Ok(_) => {}
            Err(LoleError::ObjectManagerIsNull) => {}
            Err(e) => {
                println!("main_entrypoint: error: {e:?}");
            }
        }
    }
}

#[derive(Debug)]
pub enum LoleError {
    PatchError(String),
    WindowsError(String),
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

impl<T> From<std::sync::PoisonError<T>> for LoleError {
    fn from(_: std::sync::PoisonError<T>) -> Self {
        LoleError::MutexLockError
    }
}

impl From<windows::core::Error> for LoleError {
    fn from(err: windows::core::Error) -> Self {
        Self::WindowsError(format!("{err:?}"))
    }
}

impl From<std::num::ParseIntError> for LoleError {
    fn from(err: std::num::ParseIntError) -> Self {
        LoleError::InvalidParam(format!("{err:?}"))
    }
}

#[no_mangle]
#[allow(non_snake_case, unused_variables)]
unsafe extern "system" fn DllMain(dll_module: HINSTANCE, call_reason: u32, _: *mut ()) -> bool {
    match call_reason {
        DLL_PROCESS_ATTACH => {
            let tramp = prepare_endscene_trampoline();
            if let Err(e) = tramp.enable() {
                fatal_error_exit(e);
            }
            global_var!(ENABLED_PATCHES).push(tramp);
            *global_var!(DLL_HANDLE) = dll_module;
        }
        // DLL_PROCESS_DETACH => {},
        _ => (),
    }

    true
}
