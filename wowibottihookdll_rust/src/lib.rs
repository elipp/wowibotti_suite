use core::cell::Cell;
use objectmanager::ObjectManager;
use std::sync::Mutex;
use std::time::{Duration, Instant};
use windows::Win32::System::SystemInformation::GetTickCount;
use windows::Win32::System::Threading::ExitProcess;
use windows::Win32::UI::WindowsAndMessaging::{MessageBoxW, MB_ICONERROR, MB_OK};

use lua::Opcode;
use patch::{prepare_endscene_trampoline, write_addr};
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

type Addr = usize;
type Offset = usize;

pub mod addrs;
pub mod assembly;
pub mod ctm;
pub mod lua;
pub mod objectmanager;
pub mod opcodes;
pub mod patch;
pub mod socket;
pub mod spell_error;
pub mod vec3;

use crate::ctm::prepare_ctm_finished_patch;
// use crate::ctm::prepare_ctm_finished_patch;
use crate::lua::{prepare_ClosePetStables_patch, register_lop_exec, LuaType};
use crate::patch::Patch;
use crate::socket::prepare_dump_outbound_packet_patch;
use crate::spell_error::{prepare_spell_err_msg_trampoline, SpellError};

pub const POSTGRES_ADDR: &str = "127.0.0.1:5432";
pub const POSTGRES_USER: &str = "lole";
pub const POSTGRES_PASS: &str = "lole";
pub const POSTGRES_DB: &str = "lole";

lazy_static! {
    // just in case DllMain is called from a non-main thread? :D
    pub static ref ENABLED_PATCHES: Mutex<Vec<Patch>> = Mutex::new(vec![]);
    pub static ref DLL_HANDLE: Mutex<HINSTANCE> = Mutex::new(HINSTANCE(0));
}

thread_local! {
    pub static NEED_INIT: Cell<bool> = Cell::new(true);
    pub static SHOULD_EJECT: Cell<bool> = Cell::new(false);
    pub static CONSOLE_CONOUT: Cell<HANDLE> = Cell::new(HANDLE(0));
    pub static ORIGINAL_STDOUT: Cell<HANDLE> = Cell::new(HANDLE(0));
    pub static LAST_FRAME_TIME: Cell<std::time::Instant> =
        Cell::new(std::time::Instant::now());
    pub static LAST_SPELL_ERR_MSG: Cell<Option<(SpellError, u32)>> = Cell::new(None);
    pub static LAST_FRAME_NUM: Cell<u32> = Cell::new(0);
}

pub fn wide_null(s: &str) -> Vec<u16> {
    s.encode_utf16().chain(Some(0)).collect()
}

#[macro_export]
macro_rules! global_var {
    ($name:ident) => {
        match $name.lock() {
            Ok(lock) => lock,
            Err(e) => unsafe { crate::fatal_error_exit(e.into()) },
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

fn print_as_c_array(title: &str, bytes: &[u8]) {
    println!("{title}:");
    for (i, b) in bytes.iter().enumerate() {
        print!("0x{:X}, ", b);
        if i % 10 == 9 {
            println!("");
        }
    }
    println!("");
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
    SetStdHandle(STD_OUTPUT_HANDLE, ORIGINAL_STDOUT.get())
}

unsafe fn eject_dll() -> LoleResult<()> {
    let mut patches = global_var!(ENABLED_PATCHES);
    for patch in patches.drain(..) {
        patch.disable()?;
    }
    restore_original_stdout()?;
    CloseHandle(CONSOLE_CONOUT.get())?;
    FreeConsole()?;
    std::thread::spawn(|| {
        FreeLibraryAndExitThread(*global_var!(DLL_HANDLE), 0);
    });
    Ok(())
}

const ROUGHLY_SIXTY_FPS: Duration = Duration::from_micros((950000.0 / 60.0) as u64);

fn main_entrypoint() -> LoleResult<()> {
    ctm::poll()?;
    if let Some(dt) = ROUGHLY_SIXTY_FPS.checked_sub(LAST_FRAME_TIME.get().elapsed()) {
        std::thread::sleep(dt);
    }
    Ok(())
}

#[cfg(feature = "console")]
unsafe fn open_console() -> LoleResult<()> {
    AllocConsole()?;

    let original_stdout = GetStdHandle(STD_OUTPUT_HANDLE)?;
    ORIGINAL_STDOUT.set(original_stdout);

    let conout = reopen_stdout()?;
    CONSOLE_CONOUT.set(conout);

    SetStdHandle(STD_OUTPUT_HANDLE, conout)?;
    Ok(())
}

#[cfg(not(feature = "console"))]
unsafe fn open_console() -> LoleResult<()> {
    Ok(())
}

// instead of trying to catch SIGABRT, try creating WER crash dumps:
// https://learn.microsoft.com/en-us/windows/win32/wer/collecting-user-mode-dumps?redirectedfrom=MSDN

unsafe fn initialize_dll() -> LoleResult<()> {
    open_console()?;
    let mut patches = global_var!(ENABLED_PATCHES);

    // let lua_prot = prepare_lua_prot_patch();
    // lua_prot.enable()?;
    // patches.push(lua_prot);

    // let ctm_finished = prepare_ctm_finished_patch();
    // ctm_finished.enable()?;
    // patches.push(ctm_finished);

    // let spell_err_msg = prepare_spell_err_msg_trampoline();
    // spell_err_msg.enable()?;
    // patches.push(spell_err_msg);

    // let outbound_packet_dump = prepare_dump_outbound_packet_patch();
    // outbound_packet_dump.enable()?;
    // patches.push(outbound_packet_dump);

    let closepetstables = prepare_ClosePetStables_patch();
    closepetstables.enable()?;
    patches.push(closepetstables);

    // dostring!(r#"SetCVar("realmList", "127.0.0.1")"#)?;

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

#[allow(non_snake_case)]
pub unsafe extern "stdcall" fn EndScene_hook() {
    if NEED_INIT.get() {
        if let Err(e) = initialize_dll() {
            fatal_error_exit(e);
        }
        NEED_INIT.set(false);
    } else if SHOULD_EJECT.get() {
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
    LAST_FRAME_NUM.set(LAST_FRAME_NUM.get() + 1);
    LAST_FRAME_TIME.set(Instant::now());
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
    NullPtrError,
    InvalidRawString(String),
    InvalidEnumValue(String),
    InvalidOrUnimplementedOpcodeCallNargs(Opcode, i32),
    WowSocketNotAvailable,
    PacketSynthError(String),
    SocketSendError(String),
    MutexLockError,
    StringConvError(String),
    LuaError,
    LuaUnexpectedTypeError(LuaType, LuaType),
    // DbError(postgres::Error),
    SerdeError(serde_json::Error),
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

impl From<std::ffi::NulError> for LoleError {
    fn from(err: std::ffi::NulError) -> Self {
        LoleError::StringConvError(format!("{err:?}"))
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
