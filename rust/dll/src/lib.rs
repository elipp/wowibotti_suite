use addrs::offsets::LAST_HARDWARE_ACTION;
use shared::SendSyncWrapper;
#[cfg(feature = "addonmessage_broker")]
use std::collections::VecDeque;
use std::path::{Path, PathBuf};
use windows::Win32::System::LibraryLoader::{GetProcAddress, LoadLibraryA};

use objectmanager::ObjectManager;

use socket::read_os_tick_count;
use std::collections::HashMap;
use std::ffi::c_void;
use std::str::FromStr;
use std::sync::{Arc, LazyLock, Mutex, OnceLock};
use tracing::level_filters::LevelFilter;
use tracing_subscriber::Registry;
use tracing_subscriber::layer::SubscriberExt;
use tracing_subscriber::util::SubscriberInitExt;
use windows::Win32::System::Threading::{
    ExitProcess, GetCurrentProcessId, THREAD_CREATE_RUN_IMMEDIATELY,
};
use windows::Win32::UI::WindowsAndMessaging::{
    GetForegroundWindow, GetWindowThreadProcessId, MB_ICONERROR, MB_OK, MessageBoxW,
};
use wowibotti_suite_types::{ClientConfig, RealmInfo, WowAccount};

use lua::{Opcode, lua_GetTime, lua_Integer, lua_Number};
use patch::write_addr;
use windows::Win32::Foundation::*;
use windows::Win32::Foundation::{GENERIC_READ, GENERIC_WRITE};
use windows::Win32::Storage::FileSystem::{
    CreateFileW, FILE_FLAGS_AND_ATTRIBUTES, FILE_SHARE_WRITE, OPEN_EXISTING,
};
use windows::core::{PCSTR, PCWSTR};

use windows::Win32::Foundation::HANDLE;
use windows::Win32::System::Console::AllocConsole;
use windows::Win32::System::Console::{FreeConsole, STD_OUTPUT_HANDLE};
use windows::Win32::System::Console::{GetStdHandle, SetStdHandle};
use windows::Win32::System::LibraryLoader::FreeLibraryAndExitThread;

type Addr = usize;
type Offset = isize;

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
use crate::addrs::offsets::{RENDERING_ENABLES, RENDERING_ENABLES_DEFAULT_VALUE};
use crate::lua::LuaType;
use crate::patch::{AVAILABLE_PATCHES, Patch, read_elems_from_addr};
use crate::spell_error::SpellError;

pub const POSTGRES_ADDR: &str = "127.0.0.1:5432";
pub const POSTGRES_USER: &str = "lole";
pub const POSTGRES_PASS: &str = "lole";
pub const POSTGRES_DB: &str = "lole";

#[cfg(all(not(feature = "host-windows"), not(feature = "host-linux")))]
compile_error!("one of `--feature=host-windows` or `--feature=host-linux` must be provided");

pub static ENABLED_PATCHES: LazyLock<Arc<Mutex<Vec<&'static Patch>>>> =
    LazyLock::new(|| Arc::new(Mutex::new(vec![])));

pub struct DllState {
    pub need_init: bool,
    pub should_eject: bool,
    pub console_conout: SendSyncWrapper<HANDLE>,
    pub original_stdout: SendSyncWrapper<HANDLE>,
    pub last_frame_time: lua_Number,
    pub last_frame_num: lua_Integer,
    pub last_hardware_interval: Option<std::time::Instant>,
    pub last_spell_err_msg: HashMap<SpellError, lua_Integer>,
}

pub static STATE: OnceLock<Mutex<DllState>> = OnceLock::new();

pub fn get_state() -> &'static Mutex<DllState> {
    STATE.get_or_init(|| {
        Mutex::new(DllState {
            need_init: true,
            should_eject: false,
            console_conout: SendSyncWrapper(HANDLE(std::ptr::null_mut())),
            original_stdout: SendSyncWrapper(HANDLE(std::ptr::null_mut())),
            last_frame_time: 0.0,
            last_frame_num: 0,
            last_hardware_interval: None, // initialize lazily, not at TLS init time
            last_spell_err_msg: HashMap::new(),
        })
    })
}

macro_rules! get_state_ {
    () => {
        get_state().lock().unwrap()
    };
}

static DLL_HANDLE: OnceLock<SendSyncWrapper<HMODULE>> = OnceLock::new();

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
            Err(e) => $crate::fatal_error_exit(e.into()),
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
            println!();
        }
    }
    println!();
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

fn restore_original_stdout() -> windows::core::Result<()> {
    unsafe { SetStdHandle(STD_OUTPUT_HANDLE, get_state_!().original_stdout.0) }
}

fn eject_dll() -> LoleResult<()> {
    for patch in global_var!(ENABLED_PATCHES).drain(..) {
        patch.disable()?;
    }

    restore_original_stdout()?;
    unsafe {
        CloseHandle(get_state_!().console_conout.0)?;
        FreeConsole()?;
    }

    if let Some(dll_handle) = DLL_HANDLE.get() {
        std::thread::spawn(|| unsafe {
            FreeLibraryAndExitThread(dll_handle.0, 0);
        });
    }
    Ok(())
}

const TARGET_FPS: f64 = 100.0;
const TARGET_SPF: f64 = 1.0 / TARGET_FPS; // "seconds per frame"

unsafe fn write_last_hardware_action(offset_by: i64) -> LoleResult<()> {
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
    let mut state = get_state_!();
    state.last_frame_num = state.last_frame_num + 1;
    state.last_frame_time = lua_GetTime()?;
    Ok(())
}

unsafe fn afk_refresh_hardware_event_timestamp() -> LoleResult<()> {
    let mut state = get_state_!();
    if let Some(interval) = state.last_hardware_interval
        && interval.elapsed() > std::time::Duration::from_secs(20)
    {
        unsafe {
            write_last_hardware_action(-1000)?;
        }
    }
    state.last_hardware_interval = Some(std::time::Instant::now());
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

unsafe fn disable_rendering_if_not_focused() -> LoleResult<()> {
    unsafe {
        let fg = GetForegroundWindow();
        if fg.is_invalid() {
            return Ok(());
        }

        let mut fg_pid = 0u32;
        GetWindowThreadProcessId(fg, Some(&mut fg_pid));
        if fg_pid != GetCurrentProcessId() {
            write_addr(RENDERING_ENABLES, &[0u32])?;
        } else {
            write_addr(RENDERING_ENABLES, &[RENDERING_ENABLES_DEFAULT_VALUE])?;
        }
    }
    Ok(())
}

fn main_entrypoint() -> LoleResult<()> {
    ctm::poll()?;

    // #[cfg(feature = "tbc")]
    // tbc client isn't doing any frame limiting for clients in the background
    {
        let now = lua_GetTime()?;
        let dt = now - get_state_!().last_frame_time;
        if 0.0 < dt && dt < TARGET_SPF {
            std::thread::sleep(std::time::Duration::from_secs_f64(TARGET_SPF - dt));
        }
    }

    unsafe {
        disable_rendering_if_not_focused()?;
        afk_refresh_hardware_event_timestamp()?;
    }

    set_frame_num()?;

    #[cfg(feature = "addonmessage_broker")]
    unpack_broker_message_queue();

    Ok(())
}

fn open_console() -> LoleResult<()> {
    use windows::Win32::System::Console::{
        CONSOLE_MODE, ENABLE_VIRTUAL_TERMINAL_PROCESSING, GetConsoleMode, SetConsoleMode,
    };

    unsafe {
        AllocConsole()?;
        let original_stdout = GetStdHandle(STD_OUTPUT_HANDLE)?;
        let mut state = get_state_!();

        state.original_stdout = SendSyncWrapper(original_stdout);

        let conout = reopen_stdout()?;
        state.console_conout = SendSyncWrapper(conout);

        let mut mode = CONSOLE_MODE(0);
        GetConsoleMode(conout, &mut mode)?;
        // allow ANSI color (etc.) codes
        SetConsoleMode(conout, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)?;

        SetStdHandle(STD_OUTPUT_HANDLE, conout)?;
    }

    Ok(())
}

// instead of trying to catch SIGABRT, try creating WER crash dumps:
// https://learn.microsoft.com/en-us/windows/win32/wer/collecting-user-mode-dumps?redirectedfrom=MSDN

pub(crate) struct Account(WowAccount);

impl Account {
    fn login(&self) -> LoleResult<()> {
        let sleep_duration = std::time::Duration::from_millis(rand::random::<u64>() % 4000) * 0;
        tracing::info!("Sleeping for {sleep_duration:?}");
        std::thread::sleep(sleep_duration);
        dostring!(
            "DefaultServerLogin('{}', '{}')",
            self.0.username,
            self.0.password
        );
        Ok(())
    }
}

pub(crate) struct DllRealmInfo(RealmInfo);

impl DllRealmInfo {
    fn set_realm_info(&self) -> LoleResult<()> {
        dostring!(
            "SetCVar('realmList', '{}'); SetCVar('realmName', '{}')",
            self.0.login_server,
            self.0.name
        );
        Ok(())
    }
}

pub fn get_config_file_path(identifier: &str) -> anyhow::Result<PathBuf> {
    let temp = std::env::var("TEMP")?;
    Ok(Path::new(&temp).join(format!("wow-{identifier}.json")))
}

fn read_config_from_file(identifier: &str) -> anyhow::Result<ClientConfig> {
    let config_path = get_config_file_path(identifier)?;
    match std::fs::read_to_string(&config_path) {
        Ok(contents) => match serde_json::from_str::<ClientConfig>(&contents) {
            Ok(config) => Ok(config),
            Err(err) => Err(anyhow::anyhow!("InvalidDllConfig: {err:?}")),
        },
        Err(err) => Err(anyhow::anyhow!(
            "DllConfigReadErr: ({config_path:?}) {err:?}"
        )),
    }
}

fn initialize_dll() -> LoleResult<()> {
    // open_console()?;

    let filter = tracing_subscriber::filter::LevelFilter::INFO;
    let (_, reload_handle) =
        tracing_subscriber::reload::Layer::<LevelFilter, Registry>::new(filter);
    tracing_subscriber::registry()
        .with(
            tracing_subscriber::EnvFilter::try_from_default_env()
                .unwrap_or_else(|_| tracing_subscriber::EnvFilter::new("info")),
        )
        .with(tracing_subscriber::fmt::layer().with_ansi(!cfg!(feature = "host-linux"))) // host-windows with the extended terminal capabilities actually supports ansi codes, just not wine terminal
        .init();

    let identifier = cfg_select! {
        feature = "host-linux" => { if let Ok(id) = std::env::var("LOLE_ID") { id } else { return Err(LoleError::InvalidParam(String::from("LOLE_ID missing")))} }
        feature = "host-windows" => {
            std::process::id().to_string()
        }
    };

    let config = match read_config_from_file(&identifier.to_string()) {
        Ok(config) => {
            // NOTE: this means "we're still in the login/character selection screen"
            if let Err(LoleError::ObjectManagerIsNull) = ObjectManager::new() {
                if let Some(realm) = config.realm.clone() {
                    DllRealmInfo(realm).set_realm_info()?;
                }
                if let Some(account) = config.account.clone() {
                    Account(account).login()?;
                }
            }
            if let Some(ref log_level) = config.log_level {
                let log_level = log_level.to_lowercase();
                let filter_from_config = LevelFilter::from_str(&log_level).unwrap();
                reload_handle
                    .modify(|filter| *filter = filter_from_config)
                    .unwrap();
            }

            tracing::info!("{config:#?}");
            config
        }
        Err(e) => {
            tracing::warn!("reading config failed with {e:?}");
            return Ok(());
        }
    };

    for p in CLIENT_CONFIG.get_or_init(|| config).enabled_patches.iter() {
        if let Some(patch) = AVAILABLE_PATCHES.get(p.as_str()) {
            unsafe {
                patch.enable()?;
            }
            tracing::info!("enabled patch {p}");
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
                        |msg| {
                            if let (Msg::AddonMessage(msg), Some(state)) =
                                (msg.message, BROKER_STATE.get())
                            {
                                let mut queue = state.message_queue.lock().unwrap();
                                queue.push_back(msg.clone());
                            }
                        },
                    )
                    .await
                    .unwrap();
                });
            });
        }
    }

    tracing::info!("wowibottihookdll_rust: init done! :D enabled patches:");

    for p in global_var!(ENABLED_PATCHES).iter() {
        tracing::info!("* {} @ 0x{:08X}", p.name, p.patch_addr);
    }
    Ok(())
}

fn fatal_error_exit(err: LoleError) -> ! {
    unsafe {
        MessageBoxW(
            None,
            windows_string!(&format!("{err:?}")),
            windows_string!("wowibottihookdll_rust error :("),
            MB_OK | MB_ICONERROR, // MB_OK apparently waits for the user to click OK
        );
        ExitProcess(1)
    }
}

#[allow(non_snake_case)]
pub unsafe extern "stdcall" fn EndScene_hook() {
    let need_init = get_state_!().need_init; // guard drops here at semicolon
    if need_init {
        if let Err(e) = initialize_dll() {
            // fatal_error_exit(e);
            tracing::error!("dll init failed: {e:?}");
        }
        tracing::info!("EndScene_hook: {:p}", EndScene_hook as *const ());
        get_state_!().need_init = false;
    } else if get_state_!().should_eject {
        if let Err(e) = eject_dll() {
            fatal_error_exit(e);
        }
    } else {
        match main_entrypoint() {
            Ok(_) => {}
            Err(LoleError::ObjectManagerIsNull) => {}
            Err(e) => {
                tracing::error!("main_entrypoint: error: {e:?}");
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
    MemoryWriteError(String),
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

// #[no_mangle]
// #[allow(non_snake_case, unused_variables)]
// unsafe extern "system" fn DllMain(dll_module: HMODULE, call_reason: u32, _: *mut ()) -> bool {
//     match call_reason {
//         DLL_PROCESS_ATTACH => {
//             let tramp = AVAILABLE_PATCHES.get("EndScene").expect("EndScene");
//             if let Err(e) = tramp.enable() {
//                 fatal_error_exit(e);
//             }
//             global_var!(ENABLED_PATCHES).push(tramp);
//             DLL_HANDLE.get_or_init(|| SendSyncWrapper(dll_module));
//         }
//         // DLL_PROCESS_DETACH => {},
//         _ => (),
//     }

//     true2// }

#[unsafe(no_mangle)]
pub extern "system" fn DllMain(_hinst: HMODULE, reason: u32, _reserved: *mut c_void) -> i32 {
    if reason == 1 {
        // DLL_PROCESS_ATTACH
        unsafe {
            DLL_HANDLE.get_or_init(|| SendSyncWrapper(_hinst));
            _ = windows::Win32::System::Threading::CreateThread(
                None,
                0,
                Some(main_thread),
                None,
                THREAD_CREATE_RUN_IMMEDIATELY,
                None,
            );
        }
    }
    1
}

extern "system" fn main_thread(_: *mut c_void) -> u32 {
    // all your actual code goes here
    unsafe { windows::Win32::System::Threading::Sleep(3000) }; // Sleep enough so Wow has time to initialize properly
    let tramp = AVAILABLE_PATCHES.get("EndScene").expect("EndScene");

    unsafe {
        if let Err(e) = tramp.enable() {
            fatal_error_exit(e);
        }
    }
    global_var!(ENABLED_PATCHES).push(tramp);
    0
}

struct DivxReal {
    divx_decode: unsafe extern "C" fn(i32, i32, i32) -> i32,
    initialize: unsafe extern "C" fn(i32) -> i32,
    set_output_format: unsafe extern "C" fn(i32, i32, i32, i32) -> i32,
    uninitialize: unsafe extern "C" fn(i32) -> i32,
}

static REAL: OnceLock<DivxReal> = OnceLock::new();

fn real() -> &'static DivxReal {
    REAL.get_or_init(|| unsafe {
        let lib = LoadLibraryA(PCSTR(b"DivxDecoder.dll.real\0".as_ptr())).unwrap();
        macro_rules! proc {
            ($name:literal, $type:ty) => {
                std::mem::transmute::<_, $type>(GetProcAddress(lib, PCSTR($name.as_ptr())).unwrap())
            };
        }
        DivxReal {
            divx_decode: proc!(b"DivxDecode\0", unsafe extern "C" fn(i32, i32, i32) -> i32),
            initialize: proc!(b"InitializeDivxDecoder\0", unsafe extern "C" fn(i32) -> i32),
            set_output_format: proc!(
                b"SetOutputFormat\0",
                unsafe extern "C" fn(i32, i32, i32, i32) -> i32
            ),
            uninitialize: proc!(
                b"UnInitializeDivxDecoder\0",
                unsafe extern "C" fn(i32) -> i32
            ),
        }
    })
}

macro_rules! int3h {
    () => {
        asm! {
            "int 3h"
        }
    };
}

#[unsafe(no_mangle)]
pub extern "C" fn DivxDecode(a: i32, b: i32, c: i32) -> i32 {
    // int3h!();
    unsafe { (real().divx_decode)(a, b, c) }
}
#[unsafe(no_mangle)]
pub extern "C" fn InitializeDivxDecoder(a: i32) -> i32 {
    // int3h!();
    unsafe { (real().initialize)(a) }
}
#[unsafe(no_mangle)]
pub extern "C" fn SetOutputFormat(a: i32, b: i32, c: i32, d: i32) -> i32 {
    // int3h!();
    unsafe { (real().set_output_format)(a, b, c, d) }
}
#[unsafe(no_mangle)]
pub extern "C" fn UnInitializeDivxDecoder(a: i32) -> i32 {
    // int3h!();
    unsafe { (real().uninitialize)(a) }
}
