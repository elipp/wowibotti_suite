use addrs::offsets::LAST_HARDWARE_ACTION;
use shared::SendSyncWrapper;

use std::path::{Path, PathBuf};
use std::sync::atomic::Ordering;

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
use windows::core::PCWSTR;

use windows::Win32::Foundation::HANDLE;
use windows::Win32::System::Console::AllocConsole;
use windows::Win32::System::Console::{FreeConsole, STD_OUTPUT_HANDLE};
use windows::Win32::System::Console::{GetStdHandle, SetStdHandle};
use windows::Win32::System::LibraryLoader::FreeLibraryAndExitThread;

type Addr = usize;
type Offset = isize;

pub mod addonmessage;
pub mod addrs;
pub mod assembly;
pub mod ctm;
pub mod divx;
pub mod linalg;
pub mod lua;
pub mod objectmanager;
pub mod patch;
pub mod socket;
pub mod spell_error;
pub mod wc3;
pub mod wowproto_opcodes;
#[cfg(feature = "addonmessage_broker")]
use crate::addonmessage::unpack_broker_message_queue;
use crate::addrs::offsets::{RENDERING_ENABLES, RENDERING_ENABLES_DEFAULT_VALUE};
use crate::lua::{LuaType, WORLD_ENTERED, dump_all_globals_to_file, enter_world};
use crate::patch::{AVAILABLE_PATCHES, Patch};
use crate::spell_error::SpellError;

pub mod postgres {
    pub const POSTGRES_ADDR: &str = "127.0.0.1:5432";
    pub const POSTGRES_USER: &str = "lole";
    pub const POSTGRES_PASS: &str = "lole";
    pub const POSTGRES_DB: &str = "lole";
}

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
            Err(e) => $crate::fatal_error_exit(anyhow::anyhow!("{e}")),
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

fn eject_dll() -> anyhow::Result<()> {
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

unsafe fn write_last_hardware_action(offset_by: i64) -> anyhow::Result<()> {
    let ticks = read_os_tick_count() as i64;
    write_addr::<u32>(LAST_HARDWARE_ACTION, &[(ticks + offset_by).try_into()?])?;
    Ok(())
}

fn set_frame_num() -> anyhow::Result<()> {
    let mut state = get_state_!();
    state.last_frame_num = state.last_frame_num + 1;
    state.last_frame_time = lua_GetTime()?;
    Ok(())
}

unsafe fn afk_refresh_hardware_event_timestamp() -> anyhow::Result<()> {
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

macro_rules! do_once {
    ($b:expr, $stuff:block) => {
        if let Ok(true) = $b.compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed) {
            $stuff
        }
    };
}

unsafe fn reduce_rendering_if_not_focused() -> anyhow::Result<()> {
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

#[derive(Default)]
struct LogState {
    messages: Vec<(tracing::Level, String, String)>, // (level, target, message)
}

#[derive(Clone)]
pub struct DebugLogLayer {
    state: Arc<Mutex<LogState>>,
}

impl DebugLogLayer {
    fn new() -> (Self, Arc<Mutex<LogState>>) {
        let state = Arc::new(Mutex::new(LogState::default()));
        (
            Self {
                state: Arc::clone(&state),
            },
            state,
        )
    }
}

impl<S: tracing::Subscriber> tracing_subscriber::Layer<S> for DebugLogLayer {
    fn on_event(
        &self,
        event: &tracing::Event<'_>,
        _ctx: tracing_subscriber::layer::Context<'_, S>,
    ) {
        let mut message = String::new();
        let mut visitor = StringVisitor(&mut message);
        event.record(&mut visitor);

        let meta = event.metadata();
        let level = *meta.level();
        let target = meta.target().to_string();

        if let Ok(mut state) = self.state.lock() {
            state.messages.push((level, target, message));
        }
    }
}

struct LuaPrintLayer;

struct PendingMessage {
    level: tracing::Level,
    target: String,
    message: String,
}

impl PendingMessage {
    fn print(self) {
        let level_color = match self.level {
            tracing::Level::ERROR => "FFFF5555",
            tracing::Level::WARN => "FFFFAA22",
            tracing::Level::INFO => "FF99CCAA",
            tracing::Level::DEBUG => "FF77AAFF",
            _ => "FFFFFFFF",
        };
        chatframe_print!(
            "|c{}[{}][{}] {}|r",
            level_color,
            self.level,
            self.target,
            self.message
        );
    }
}

static PENDING_MESSAGES: Mutex<Vec<PendingMessage>> = Mutex::new(Vec::new());

impl<S: tracing::Subscriber> tracing_subscriber::Layer<S> for LuaPrintLayer {
    fn on_event(
        &self,
        event: &tracing::Event<'_>,
        _ctx: tracing_subscriber::layer::Context<'_, S>,
    ) {
        let mut message = String::new();
        event.record(&mut StringVisitor(&mut message));

        let meta = event.metadata();
        let level = *meta.level();

        let mut pending = PENDING_MESSAGES.lock().unwrap();
        pending.push(PendingMessage {
            level,
            target: meta.target().to_string(),
            message,
        });
    }
}

struct StringVisitor<'a>(&'a mut String);

impl<'a> tracing::field::Visit for StringVisitor<'a> {
    fn record_debug(&mut self, field: &tracing::field::Field, value: &dyn std::fmt::Debug) {
        if field.name() == "message" {
            *self.0 = format!("{:?}", value);
        }
    }
}

// This must occur on the main thread, otherwise Wow gets "Fatal condition" errors
fn flush_tracing_log_queue() {
    if WORLD_ENTERED.load(Ordering::Relaxed) {
        let mut pending = PENDING_MESSAGES.lock().unwrap();
        for msg in pending.drain(..) {
            msg.print();
        }
    }
}

fn on_every_frame() -> anyhow::Result<()> {
    enter_world()?;

    match ctm::poll() {
        Ok(_) => {}
        Err(e) if let Some(LoleError::PlayerNotFound) = e.downcast_ref() => {}
        Err(e) => return Err(e),
    }

    unsafe {
        reduce_rendering_if_not_focused()?;
        afk_refresh_hardware_event_timestamp()?;
    }

    set_frame_num()?;

    #[cfg(feature = "addonmessage_broker")]
    unpack_broker_message_queue();

    flush_tracing_log_queue();

    Ok(())
}

fn open_console() -> anyhow::Result<()> {
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
    fn login(&self) -> anyhow::Result<()> {
        // let sleep_duration = std::time::Duration::from_millis(rand::random::<u64>() % 4000) * 0;
        // tracing::info!("Sleeping for {sleep_duration:?}");
        // std::thread::sleep(sleep_duration);
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
    fn set_realm_info(&self) -> anyhow::Result<()> {
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

fn initialize_dll() -> anyhow::Result<()> {
    let Ok(lole_id) = std::env::var("LOLE_ID") else {
        return Err(LoleError::InvalidParam(String::from("LOLE_ID missing")))?;
    };

    #[cfg(feature = "host-windows")]
    open_console()?;

    let filter = tracing_subscriber::filter::LevelFilter::INFO;
    let (_, reload_handle) =
        tracing_subscriber::reload::Layer::<LevelFilter, Registry>::new(filter);

    // let (debug_log_layer, state) = DebugLogLayer::new();

    use tracing_subscriber::prelude::*;

    // let file = std::fs::File::create(format!("/tmp/log-{lole_id}.txt"))
    //     .expect("failed to create log file");

    tracing_subscriber::registry()
        .with(
            tracing_subscriber::EnvFilter::try_from_default_env()
                .unwrap_or_else(|_| tracing_subscriber::EnvFilter::new("info")),
        )
        .with(tracing_subscriber::fmt::layer().with_ansi(!cfg!(feature = "host-linux"))) // host-windows with the extended terminal capabilities actually supports ansi codes, just not wine terminal
        // .with(
        //     tracing_subscriber::fmt::layer()
        //         .with_ansi(false)
        //         .with_writer(file),
        // )
        .with(LuaPrintLayer)
        // .with(debug_log_layer)
        .init();

    let config = match read_config_from_file(&lole_id) {
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
            tracing::warn!("reading config failed with {e}");
            return Ok(());
        }
    };

    let config = CLIENT_CONFIG.get_or_init(|| config);

    for p in config.enabled_patches.iter() {
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
        if let Some(character_name) = get_current_character_name()
            && let Some(addr) = config.addonmessage_broker_addr.clone()
        {
            use crate::addonmessage::start_client;

            start_client(character_name, addr);
        }
    }

    tracing::info!("init done! :D LOLE_ID: {lole_id} - enabled patches:");

    for p in global_var!(ENABLED_PATCHES).iter() {
        tracing::info!("{} @ 0x{:08X}", p.name, p.patch_addr);
    }
    Ok(())
}

fn fatal_error_exit(err: anyhow::Error) -> ! {
    unsafe {
        MessageBoxW(
            None,
            windows_string!(&format!("{err}")),
            windows_string!("lole error :("),
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
            tracing::error!("dll init failed: {e}");
        }
        get_state_!().need_init = false;
    } else if get_state_!().should_eject {
        if let Err(e) = eject_dll() {
            fatal_error_exit(e);
        }
    } else {
        match on_every_frame() {
            Ok(_) => {}
            Err(anyhow_err) => match anyhow_err.downcast_ref::<LoleError>() {
                Some(e) if let LoleError::ObjectManagerIsNull = e => {
                    // this error is the only one that's fine
                }
                _ => {
                    fatal_error_exit(anyhow_err);
                }
            },
        }
    }
}

#[derive(Debug, thiserror::Error)]
pub enum LoleError {
    #[error("Patch error: {0}")]
    PatchError(String),
    #[error("Windows error: {0}")]
    WindowsError(String),
    #[error("Client connection is null")]
    ClientConnectionIsNull,
    #[error("Object manager is null")]
    ObjectManagerIsNull,
    #[error("Player not found")]
    PlayerNotFound,
    #[error("Invalid param: {0}")]
    InvalidParam(String),
    #[error("Invalid WoW object type")]
    InvalidWowObjectType,
    #[error("Memory write error: {0}")]
    MemoryWriteError(String),
    #[error("Partial memory write error")]
    PartialMemoryWriteError,
    #[error("Lua state is null")]
    LuaStateIsNull,
    #[error("Missing opcode")]
    MissingOpcode,
    #[error("Unknown opcode: {0}")]
    UnknownOpcode(i32),
    #[error("Null pointer error")]
    NullPtrError,
    #[error("Invalid raw string: {0}")]
    InvalidRawString(String),
    #[error("Invalid enum value: {0}")]
    InvalidEnumValue(String),
    #[error("Invalid or unimplemented opcode call nargs: {0:?}, {1}")]
    InvalidOrUnimplementedOpcodeCallNargs(Opcode, i32),
    #[error("WoW socket not available")]
    WowSocketNotAvailable,
    #[error("Packet synth error: {0}")]
    PacketSynthError(String),
    #[error("Socket send error: {0}")]
    SocketSendError(String),
    #[error("Mutex lock error")]
    MutexLockError,
    #[error("String conversion error: {0}")]
    StringConvError(String),
    #[error("Lua error: {0}")]
    LuaError(String),
    #[error("Lua unexpected type error: expected {0:?}, got {1:?}")]
    LuaUnexpectedTypeError(LuaType, LuaType),
    // DbError(postgres::Error),
    #[error("Serde error: {0}")]
    SerdeError(serde_json::Error),
    #[error("Invalid DLL config: {0}")]
    InvalidDllConfig(String),
    #[error("DLL config read error: {0}")]
    DllConfigReadError(String),
    #[error("Missing credentials")]
    MissingCredentials,
    #[error("Not implemented")]
    NotImplemented,
    #[error("Unknown branch taken")]
    UnknownBranchTaken,
}

pub type LoleResult<T> = Result<T, LoleError>;

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
    if let Ok(_) = std::env::var("LOLE_ID") {
        unsafe { windows::Win32::System::Threading::Sleep(3000) }; // Sleep enough so Wow has time to initialize properly (sometimes even this isn't enough)
        let tramp = AVAILABLE_PATCHES.get("EndScene").expect("EndScene");

        // there's a tiny chance the tramp patching occurs exactly as EndScene is executing, but hey
        unsafe {
            if let Err(e) = tramp.enable() {
                fatal_error_exit(e.into());
            }
        }
        global_var!(ENABLED_PATCHES).push(tramp);
    }
    0
}
