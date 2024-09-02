use addonmessage_broker::SendSyncWrapper;
use addrs::offsets::LAST_HARDWARE_ACTION;
use core::cell::Cell;
use objectmanager::ObjectManager;
use serde::{Deserialize, Serialize};
use socket::read_os_tick_count;
use std::cell::RefCell;
use std::collections::{HashMap, VecDeque};
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex, OnceLock};
use std::time::{Duration, Instant};
use windows::Win32::System::Threading::ExitProcess;
use windows::Win32::UI::WindowsAndMessaging::{MessageBoxW, MB_ICONERROR, MB_OK};

use lua::{
    get_wow_lua_state, lua_GetTime, lua_Integer, lua_Number, lua_debug_func, playermode, Opcode,
};
use patch::write_addr;
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
pub mod patch;
pub mod socket;
pub mod spell_error;
pub mod vec3;
pub mod wowproto_opcodes;

use crate::lua::LuaType;
use crate::patch::{Patch, AVAILABLE_PATCHES};
use crate::spell_error::SpellError;
use tokio::task;

pub const POSTGRES_ADDR: &str = "127.0.0.1:5432";
pub const POSTGRES_USER: &str = "lole";
pub const POSTGRES_PASS: &str = "lole";
pub const POSTGRES_DB: &str = "lole";

lazy_static! {
    pub static ref ENABLED_PATCHES: Arc<Mutex<Vec<&'static Patch>>> = Arc::new(Mutex::new(vec![]));
}

thread_local! {
    pub static NEED_INIT: Cell<bool> = Cell::new(true);
    pub static SHOULD_EJECT: Cell<bool> = Cell::new(false);
    pub static CONSOLE_CONOUT: Cell<HANDLE> = Cell::new(HANDLE(std::ptr::null_mut()));
    pub static ORIGINAL_STDOUT: Cell<HANDLE> = Cell::new(HANDLE(std::ptr::null_mut()));
    pub static LAST_FRAME_TIME: Cell<lua_Number> = Cell::new(0.0);

    pub static LAST_SPELL_ERR_MSG: RefCell<HashMap<SpellError, lua_Integer>> = RefCell::new(Default::default());
    pub static LAST_FRAME_NUM: Cell<lua_Integer> = Cell::new(0);

    pub static LAST_HARDWARE_INTERVAL: Cell<std::time::Instant> = Cell::new(std::time::Instant::now());
    pub static LOCAL_SET: RefCell<task::LocalSet> = RefCell::new(task::LocalSet::new());
}

static DLL_HANDLE: OnceLock<SendSyncWrapper<HINSTANCE>> = OnceLock::new();

#[cfg(feature = "addonmessage_broker")]
use addonmessage_broker::{
    client::start_addonmessage_client, server::AddonMessage, server::ConnectionId, server::Msg,
    server::MsgWrapper,
};

// #[cfg(feature = "addonmessage_broker")]
// pub static BROKER_CONNECTION_ID: OnceLock<ConnectionId> = OnceLock::new();
// #[cfg(feature = "addonmessage_broker")]
// pub static BROKER_TX: OnceLock<std::sync::mpsc::Sender<MsgWrapper>> = OnceLock::new();
// #[cfg(feature = "addonmessage_broker")]
// lazy_static! {
//     pub static ref BROKER_MESSAGE_QUEUE: Arc<Mutex<VecDeque<AddonMessage>>> =
//         Arc::new(Mutex::new(VecDeque::new()));
// }

#[cfg(feature = "addonmessage_broker")]
pub struct BrokerState {
    pub connection_id: ConnectionId,
    pub character_name: String,
    pub tx: std::sync::mpsc::Sender<MsgWrapper>,
    pub message_queue: Arc<Mutex<VecDeque<AddonMessage>>>,
}

#[cfg(feature = "addonmessage_broker")]
pub static BROKER_STATE: OnceLock<BrokerState> = OnceLock::new();

pub static CLIENT_CONFIG: OnceLock<ClientConfig> = OnceLock::new();

pub fn get_current_character_name() -> Option<String> {
    if let Some(config) = CLIENT_CONFIG.get() {
        if let Some(ref account) = config.account {
            return Some(account.character.name.clone());
        }
    }
    None
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

#[macro_export]
macro_rules! windows_string {
    ($s:expr) => {
        PCWSTR::from_raw(
            $s.encode_utf16()
                .chain(Some(0))
                .collect::<Vec<u16>>()
                .as_ptr(),
        )
    };
}

fn print_as_c_array(title: &str, bytes: &[u8]) {
    println!("{title}:");
    for (i, b) in bytes.iter().enumerate() {
        print!("0x{:02X}, ", b);
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
    for patch in global_var!(ENABLED_PATCHES).drain(..) {
        patch.disable()?;
    }
    restore_original_stdout()?;
    CloseHandle(CONSOLE_CONOUT.get())?;
    FreeConsole()?;
    if let Some(dll_handle) = DLL_HANDLE.get() {
        std::thread::spawn(|| {
            FreeLibraryAndExitThread(dll_handle.0, 0);
        });
    }
    Ok(())
}

const ROUGHLY_SIXTY_FPS: Duration = Duration::from_micros((950000.0 / 60.0) as u64);

fn write_last_hardware_action(offset_by: i64) -> LoleResult<()> {
    let ticks = read_os_tick_count() as i64;
    write_addr::<u32>(
        LAST_HARDWARE_ACTION,
        &[(ticks + offset_by)
            .try_into()
            .map_err(|_e| LoleError::InvalidParam(format!("ticks + {offset_by}")))?],
    )?;
    Ok(())
}

fn set_frame_num() -> LoleResult<()> {
    LAST_FRAME_NUM.set(LAST_FRAME_NUM.get() + 1);
    LAST_FRAME_TIME.set(lua_GetTime()?);
    Ok(())
}

fn refresh_hardware_event_timestamp() -> LoleResult<()> {
    if LAST_HARDWARE_INTERVAL.get().elapsed() > std::time::Duration::from_secs(20) {
        LAST_HARDWARE_INTERVAL.set(std::time::Instant::now());
        write_last_hardware_action(-1000)?;
    }
    Ok(())
}

#[cfg(feature = "addonmessage_broker")]
fn unpack_broker_message_queue() {
    if let Some(state) = BROKER_STATE.get() {
        let mut queue = state.message_queue.lock().unwrap();
        for message in queue.drain(..) {
            let script = format!(
                "addonmessage_received([[{}]], [[{}]], {}, [[{}]])",
                message.prefix,
                message.text,
                message
                    .r#type
                    .map(|s| format!("[[{}]]", s))
                    .unwrap_or_else(|| "nil".to_string()),
                message.from,
            );
            dostring!(script);
        }
    }
}

fn main_entrypoint() -> LoleResult<()> {
    ctm::poll()?;

    #[cfg(feature = "tbc")]
    // tbc client isn't doing any frame limiting for clients in the background
    if let Some(dt) = ROUGHLY_SIXTY_FPS.checked_sub(LAST_FRAME_TIME.get().elapsed()) {
        std::thread::sleep(dt);
    }

    refresh_hardware_event_timestamp()?;
    set_frame_num()?;

    #[cfg(feature = "addonmessage_broker")]
    unpack_broker_message_queue();

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

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum CharacterClass {
    Druid,
    Hunter,
    DeathKnight,
    Paladin,
    Priest,
    Rogue,
    Mage,
    Shaman,
    Warlock,
    Warrior,
}

impl std::fmt::Display for CharacterClass {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "{:?}", self)
    }
}
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CharacterInfo {
    pub name: String,
    pub class: CharacterClass,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct WowAccount {
    pub username: String,
    pub password: String,
    pub character: CharacterInfo,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct RealmInfo {
    pub login_server: String,
    pub name: String,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct ClientConfig {
    pub realm: Option<RealmInfo>,
    pub account: Option<WowAccount>,
    pub enabled_patches: Vec<String>,
}

impl WowAccount {
    fn login(&self) -> LoleResult<()> {
        let sleep_duration = std::time::Duration::from_millis(rand::random::<u64>() % 4000);
        println!("Sleeping for {sleep_duration:?}");
        std::thread::sleep(sleep_duration);
        dostring!(
            "DefaultServerLogin('{}', '{}')",
            self.username,
            self.password
        );
        Ok(())
    }
    pub fn write_config_to_tmp_file(
        &self,
        pid: u32,
        enabled_patches: Vec<String>,
    ) -> Result<(), String> {
        let full_path = get_config_file_path(pid).map_err(|e| format!("Error: {e:?}"))?;

        let client_config = ClientConfig {
            realm: Some(RealmInfo {
                login_server: String::from("logon.warmane.com"),
                name: String::from("Lordaeron"),
            }),
            account: Some(self.clone()),
            enabled_patches,
        };

        let serialized = serde_json::to_string(&client_config).map_err(|_e| format!("{_e:?}"))?;
        std::fs::write(&full_path, &serialized).map_err(|_e| format!("{_e:?}"))?;

        Ok(())
    }
}

impl RealmInfo {
    fn set_realm_info(&self) -> LoleResult<()> {
        dostring!(
            "SetCVar('realmList', '{}'); SetCVar('realmName', '{}')",
            self.login_server,
            self.name
        );
        Ok(())
    }
}

pub fn get_config_file_path(pid: u32) -> Result<PathBuf, String> {
    let localappdata =
        std::env::var("LOCALAPPDATA").map_err(|_e| String::from("LOCALAPPDATA missing"))?;
    Ok(Path::new(&localappdata)
        .join("Temp")
        .join(&format!("wow-{pid}.json")))
}

fn read_config_from_file(pid: u32) -> Result<ClientConfig, String> {
    let config_path = get_config_file_path(pid)?;
    match std::fs::read_to_string(&config_path) {
        Ok(contents) => match serde_json::from_str::<ClientConfig>(&contents) {
            Ok(config) => Ok(config),
            Err(err) => Err(format!("InvalidDllConfig: {err:?}")),
        },
        Err(err) => Err(format!("DllConfigReadErr: {err:?}")),
    }
}

unsafe fn initialize_dll() -> LoleResult<()> {
    open_console()?;

    let config = match read_config_from_file(std::process::id()) {
        Ok(config) => {
            // NOTE: this means "we're still in the login/character selection screen"
            if let Err(LoleError::ObjectManagerIsNull) = ObjectManager::new() {
                if let Some(ref realm) = config.realm {
                    realm.set_realm_info()?;
                }
                if let Some(ref creds) = config.account {
                    creds.login()?;
                }
            }
            config
        }
        Err(e) => {
            panic!("warning: reading config failed with {e:?}");
        }
    };

    for p in CLIENT_CONFIG.get_or_init(|| config).enabled_patches.iter() {
        if let Some(patch) = AVAILABLE_PATCHES.get(p.as_str()) {
            patch.enable()?;
            eprintln!("enabled patch {p}");
            global_var!(ENABLED_PATCHES).push(patch);
        } else {
            panic!("unknown patch {p}");
        }
    }

    #[cfg(feature = "addonmessage_broker")]
    {
        if let Some(character_name) = get_current_character_name() {
            let (tx, rx) = std::sync::mpsc::channel::<MsgWrapper>();
            let _handle = std::thread::spawn(move || {
                let rt = tokio::runtime::Runtime::new().unwrap();
                rt.block_on(async {
                    start_addonmessage_client(
                        rx,
                        character_name.clone(),
                        |connection_id| {
                            BROKER_STATE.get_or_init(|| BrokerState {
                                connection_id,
                                character_name,
                                tx,
                                message_queue: Default::default(),
                            });
                        },
                        |msg| match (msg.message, BROKER_STATE.get()) {
                            (Msg::AddonMessage(msg), Some(state)) => {
                                let mut queue = state.message_queue.lock().unwrap();
                                queue.push_back(msg.clone());
                            }
                            _ => {}
                        },
                    )
                    .await
                    .unwrap();
                });
            });
        }
    }

    println!("wowibottihookdll_rust: init done! :D enabled patches:");

    for p in global_var!(ENABLED_PATCHES).iter() {
        println!("* {} @ 0x{:08X}", p.name, p.patch_addr);
    }
    Ok(())
}

unsafe fn fatal_error_exit(err: LoleError) -> ! {
    MessageBoxW(
        HWND(std::ptr::null_mut()),
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
}

#[derive(Debug)]
pub enum LoleError {
    PatchError(String),
    WindowsError(String),
    ClientConnectionIsNull,
    ObjectManagerIsNull,
    PlayerNotFound,
    InvalidParam(String),
    InvalidWowObjectType,
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
    LuaError(String),
    LuaUnexpectedTypeError(LuaType, LuaType),
    // DbError(postgres::Error),
    SerdeError(serde_json::Error),
    InvalidDllConfig(String),
    DllConfigReadError(String),
    MissingCredentials,
    NotImplemented,
    UnknownBranchTaken,
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
            let tramp = AVAILABLE_PATCHES.get("EndScene").expect("EndScene");
            if let Err(e) = tramp.enable() {
                fatal_error_exit(e);
            }
            global_var!(ENABLED_PATCHES).push(tramp);
            DLL_HANDLE.get_or_init(|| SendSyncWrapper(dll_module));
        }
        // DLL_PROCESS_DETACH => {},
        _ => (),
    }

    true
}
