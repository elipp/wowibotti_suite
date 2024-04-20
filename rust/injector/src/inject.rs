use lazy_static::lazy_static;
use serde::Serialize;
use std::ffi::c_void;
use std::os::windows::ffi::OsStrExt;
use std::sync::{Arc, Mutex};

use wowibottihookdll::{get_config_file_path, ClientConfig, RealmInfo};

use windows::{
    core::{s, w, PCWSTR},
    Win32::{
        Foundation::{CloseHandle, BOOL, FALSE, HANDLE, HWND, LPARAM, LUID},
        Security::{
            AdjustTokenPrivileges, ImpersonateSelf, LookupPrivilegeValueW, SecurityImpersonation,
            SE_DEBUG_NAME, SE_PRIVILEGE_ENABLED, TOKEN_ADJUST_PRIVILEGES, TOKEN_PRIVILEGES,
            TOKEN_QUERY,
        },
        System::{
            Diagnostics::Debug::WriteProcessMemory,
            LibraryLoader::{GetModuleHandleW, GetProcAddress},
            Memory::{VirtualAllocEx, MEM_COMMIT, MEM_RESERVE, PAGE_READWRITE},
            Threading::{
                CreateRemoteThread, GetCurrentThread, GetExitCodeThread, OpenProcess,
                OpenThreadToken, WaitForSingleObject, INFINITE, PROCESS_ALL_ACCESS,
            },
        },
        UI::WindowsAndMessaging::{
            EnumWindows, GetClassNameW, GetWindowTextW, GetWindowThreadProcessId,
        },
    },
};

use crate::{InjectQuery, PottiConfig, WowAccount};

lazy_static! {
    static ref CLIENTS: Arc<Mutex<Vec<WowClient>>> = Arc::new(Mutex::new(Vec::new()));
}

pub type InjectorResult<T> = Result<T, InjectorError>;

#[derive(Debug)]
pub enum InjectorError {
    DebugPrivilegesFailed(String),
    SerializationError(String),
    DeserializationError(String),
    InjectionError(String),
    NotFound,
    HttpError(http::Error),
    CharacterEntryNotFound(String),
    LaunchError(String),
    OtherError(String),
}

impl From<http::Error> for InjectorError {
    fn from(e: http::Error) -> Self {
        Self::HttpError(e)
    }
}

#[derive(Debug, Clone)]
pub struct WowClient {
    window_handle: HWND,
    window_title: String,
    class_name: String,
    pid: u32,
    tid: u32, // tid of window thread
    index: usize,
    library_handle: Option<HANDLE>,
}

#[derive(Debug, Clone, Serialize)]
pub struct WowClientInfo {
    pid: u32,
    tid: u32,
    index: usize,
}

impl From<WowClient> for WowClientInfo {
    fn from(c: WowClient) -> Self {
        Self {
            pid: c.pid,
            tid: c.tid,
            index: c.index,
        }
    }
}

impl WowClient {
    fn write_config_to_tmp_file(&self, account: WowAccount) -> InjectorResult<()> {
        let full_path = get_config_file_path(self.pid).map_err(|e| InjectorError::OtherError(e))?;

        let client_config = ClientConfig {
            realm: Some(RealmInfo {
                login_server: String::from("logon.warmane.com"),
                name: String::from("Lordaeron"),
            }),
            account: Some(account),
            enabled_patches: None,
        };

        let serialized = serde_json::to_string(&client_config)
            .map_err(|_e| InjectorError::SerializationError(format!("{_e:?}")))?;

        std::fs::write(&full_path, &serialized)
            .map_err(|_e| InjectorError::OtherError(format!("{_e:?}")))?;

        Ok(())
    }

    fn inject(&self, account: Option<WowAccount>) -> InjectorResult<WowClientInfo> {
        let client = self.clone();
        let modified_client = std::thread::spawn(move || unsafe {
            obtain_debug_privileges()?;
            if let Some(account) = account {
                client.write_config_to_tmp_file(account)?;
            }

            let process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, client.pid).unwrap();

            let load_library_addr = GetProcAddress(
                GetModuleHandleW(w!("kernel32.dll")).map_err(|_e| {
                    InjectorError::InjectionError(String::from("GetModuleHandleW"))
                })?,
                s!("LoadLibraryW"),
            )
            .ok_or_else(|| InjectorError::InjectionError(String::from("GetProcAddress")))?;

            let dll_path = std::env::current_dir()
                .unwrap()
                .join("target")
                .join("i686-pc-windows-msvc")
                .join("debug")
                .join("wowibottihookdll.dll");

            if !dll_path.exists() {
                return Err(InjectorError::InjectionError(format!(
                    "dll file `{}` doesn't exist",
                    dll_path.display()
                )));
            }

            let as_u16: Vec<u16> = dll_path.as_os_str().encode_wide().collect();
            let path_len_bytes = as_u16.len() * std::mem::size_of::<i16>();

            let llparam = VirtualAllocEx(
                process,
                None,
                path_len_bytes,
                MEM_RESERVE | MEM_COMMIT,
                PAGE_READWRITE,
            );

            if llparam.is_null() {
                return Err(InjectorError::InjectionError(String::from(
                    "VirtualAllocEx",
                )));
            }

            WriteProcessMemory(
                process,
                llparam,
                as_u16.as_ptr() as *const c_void,
                path_len_bytes,
                None,
            )
            .map_err(|_e| InjectorError::InjectionError(String::from("WriteProcessMemory")))?;

            let load_library_addr = std::mem::transmute::<
                _,
                unsafe extern "system" fn(*mut c_void) -> u32,
            >(load_library_addr as *const ());

            let rt = CreateRemoteThread(
                process,
                None,
                0,
                Some(load_library_addr),
                Some(llparam),
                0,
                None,
            )
            .map_err(|_e| InjectorError::InjectionError(String::from("CreateRemoteThread")))?;

            WaitForSingleObject(rt, INFINITE);
            let mut library_handle = 0u32;
            GetExitCodeThread(rt, &mut library_handle as *mut u32)
                .map_err(|_e| InjectorError::InjectionError(String::from("GetExitCodeThread")))?;
            CloseHandle(process).unwrap();
            if library_handle == 0 {
                Err(InjectorError::InjectionError(String::from("LoadLibraryW")))
            } else {
                Ok::<WowClientInfo, InjectorError>(client.into())
            }
        })
        .join()
        .map_err(|e| InjectorError::InjectionError(format!("{e:?}")))?
        .map_err(|e| InjectorError::InjectionError(format!("{e:?}")))?;
        Ok(modified_client)
    }
}

unsafe fn obtain_debug_privileges() -> InjectorResult<()> {
    let mut token: HANDLE = Default::default();
    ImpersonateSelf(SecurityImpersonation)
        .map_err(|_e| InjectorError::DebugPrivilegesFailed(format!("ImpersonateSelf: {_e:?}")))?;
    OpenThreadToken(
        GetCurrentThread(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
        false,
        &mut token as *mut HANDLE,
    )
    .map_err(|_e| InjectorError::DebugPrivilegesFailed(format!("OpenThreadToken: {_e:?}")))?;

    let mut luid: LUID = Default::default();
    LookupPrivilegeValueW(None, SE_DEBUG_NAME, &mut luid as *mut LUID).map_err(|_e| {
        InjectorError::DebugPrivilegesFailed(format!("LookupPrivilegeValueW: {_e:?}"))
    })?;

    let mut privileges: TOKEN_PRIVILEGES = Default::default();
    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Luid = luid;
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(
        token,
        false,
        Some(&privileges as *const TOKEN_PRIVILEGES),
        std::mem::size_of::<TOKEN_PRIVILEGES>() as u32,
        None,
        None,
    )
    .map_err(|_e| InjectorError::DebugPrivilegesFailed(format!("AdjustTokenPrivileges: {_e:?}")))?;

    return Ok(());
}

unsafe extern "system" fn enum_windows_proc(hwnd: HWND, _lparam: LPARAM) -> BOOL {
    let mut buf = vec![0u16; 1024];
    GetClassNameW(hwnd, &mut buf);
    let class_name = PCWSTR::from_raw(buf.as_ptr()).to_string().unwrap();

    GetWindowTextW(hwnd, &mut buf);
    let window_title = PCWSTR::from_raw(buf.as_ptr()).to_string().unwrap();

    let mut pid = 0u32;
    let tid = GetWindowThreadProcessId(hwnd, Some(&mut pid as *mut u32));

    let mut clients = CLIENTS.lock().unwrap();

    if window_title == "World of Warcraft" {
        let index = clients.len();
        clients.push(WowClient {
            window_handle: hwnd,
            window_title,
            class_name,
            pid,
            tid,
            index,
            library_handle: None,
        });
    }

    true.into()
}

fn inject_dll(
    clients: &Vec<WowClient>,
    config: PottiConfig,
    query: InjectQuery,
) -> InjectorResult<Vec<WowClientInfo>> {
    let mut res = Vec::default();
    for (client, name) in clients.iter().zip(query.enabled_characters.iter()) {
        let account = config
            .accounts
            .iter()
            .find(|a| &a.character.name == name)
            .ok_or_else(|| InjectorError::CharacterEntryNotFound(name.to_owned()))?;
        res.push(client.inject(Some(account.clone()))?);
    }
    Ok(res)
}

fn enum_windows() {
    CLIENTS.lock().unwrap().clear();
    unsafe { EnumWindows(Some(enum_windows_proc), LPARAM::default()) }.unwrap();
}

pub fn find_wow_windows_and_inject(
    config: PottiConfig,
    query: InjectQuery,
) -> InjectorResult<Vec<WowClientInfo>> {
    enum_windows();
    let clients = CLIENTS.lock().unwrap();
    inject_dll(&clients, config, query)
}
