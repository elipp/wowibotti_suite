use addonmessage_broker::SendSyncWrapper;
use lazy_static::lazy_static;
use serde::Serialize;
use std::ffi::c_void;
use std::sync::{Arc, Mutex};
use windows::core::BOOL;
use windows::Win32::Foundation::WPARAM;
use windows::Win32::UI::WindowsAndMessaging::{SendMessageW, WM_USER};

use windows::{
    core::{s, w, PCWSTR},
    Win32::{
        Foundation::{CloseHandle, HANDLE, HWND, LPARAM, LUID},
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

use crate::{str_into_vec_u16, InjectQuery, PottiConfig, WowAccount, DUMMY_WINDOW_HWND};

pub const INJ_MESSAGE_REGISTER_HOTKEY: u32 = WM_USER;
pub const INJ_MESSAGE_UNREGISTER_HOTKEY: u32 = WM_USER + 1;

lazy_static! {
    pub static ref CLIENTS: Arc<Mutex<Vec<WowClient>>> = Arc::new(Mutex::new(Vec::new()));
}

pub type InjectorResult<T> = Result<T, InjectorError>;

#[derive(Debug)]
pub enum InjectorError {
    DebugPrivilegesFailed(String),
    SerializationError(String),
    DeserializationError(String),
    InjectionError(String),
    NotFound,
    #[cfg(feature = "web")]
    HttpError(http::Error),
    CharacterEntryNotFound(String),
    LaunchError(String),
    HotkeyError(String),
    OtherError(String),
}

#[cfg(feature = "web")]
impl From<http::Error> for InjectorError {
    fn from(e: http::Error) -> Self {
        Self::HttpError(e)
    }
}

#[derive(Debug)]
pub struct WowClient {
    pub window_handle: HWND,
    window_title: String,
    class_name: String,
    pid: u32,
    tid: u32, // tid of window thread
    index: i32,
    library_handle: Option<HANDLE>,
}

unsafe impl Sync for WowClient {}
unsafe impl Send for WowClient {}

impl Drop for WowClient {
    fn drop(&mut self) {
        self.unregister_hotkey().unwrap();
    }
}

#[derive(Debug, Clone, Serialize)]
pub struct WowClientInfo {
    pid: u32,
    tid: u32,
    index: i32,
}

impl From<&WowClient> for WowClientInfo {
    fn from(c: &WowClient) -> Self {
        Self {
            pid: c.pid,
            tid: c.tid,
            index: c.index,
        }
    }
}

impl WowClient {
    fn register_hotkey(&self) -> InjectorResult<()> {
        let index_str = str_into_vec_u16((self.index + 1).rem_euclid(10).to_string());
        if index_str.len() < 1 {
            return Err(InjectorError::HotkeyError(format!(
                "Hotkey vkeycode conversion failed"
            )));
        }
        if let Some(SendSyncWrapper(hwnd)) = DUMMY_WINDOW_HWND.get() {
            unsafe {
                SendMessageW(
                    *hwnd,
                    INJ_MESSAGE_REGISTER_HOTKEY,
                    Some(WPARAM(self.index as usize)),
                    Some(LPARAM(index_str[0] as isize)),
                );
            }
            Ok(())
        } else {
            Err(InjectorError::HotkeyError(format!(
                "register_hotkey: DUMMY_WINDOW not set"
            )))
        }
    }

    fn unregister_hotkey(&self) -> InjectorResult<()> {
        if let Some(SendSyncWrapper(hwnd)) = DUMMY_WINDOW_HWND.get() {
            unsafe {
                SendMessageW(
                    *hwnd,
                    INJ_MESSAGE_UNREGISTER_HOTKEY,
                    Some(WPARAM(self.index as usize)),
                    None, // used to be LPARAM(0)
                )
            };
            Ok(())
        } else {
            Err(InjectorError::HotkeyError(format!(
                "unregister_hotkey: DUMMY_WINDOW not set"
            )))
        }
    }

    fn inject(
        &self,
        account: Option<WowAccount>,
        query: &InjectQuery,
    ) -> InjectorResult<WowClientInfo> {
        let pid = self.pid;
        let client_info = self.into();
        let enabled_patches = query.enabled_patches.clone();
        let modified_client = std::thread::spawn(move || unsafe {
            obtain_debug_privileges()?;

            if let Some(account) = account {
                account
                    .write_config_to_tmp_file(pid, enabled_patches)
                    .map_err(|s| InjectorError::OtherError(s))?;
            }

            let process = OpenProcess(PROCESS_ALL_ACCESS, false, pid).unwrap();

            let load_library_addr = GetProcAddress(
                GetModuleHandleW(w!("kernel32.dll")).map_err(|_e| {
                    InjectorError::InjectionError(String::from("GetModuleHandleW"))
                })?,
                s!("LoadLibraryW"),
            )
            .ok_or_else(|| InjectorError::InjectionError(String::from("GetProcAddress")))?;

            let dll_path = std::env::current_dir().unwrap().join("lolerust.dll");

            if !dll_path.exists() {
                return Err(InjectorError::InjectionError(format!(
                    "dll file `{}` doesn't exist",
                    dll_path.display()
                )));
            }

            let as_u16: Vec<u16> = str_into_vec_u16(dll_path);
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
            GetExitCodeThread(rt, &mut library_handle)
                .map_err(|_e| InjectorError::InjectionError(String::from("GetExitCodeThread")))?;
            CloseHandle(process).unwrap();
            if library_handle == 0 {
                Err(InjectorError::InjectionError(String::from("LoadLibraryW")))
            } else {
                Ok::<WowClientInfo, InjectorError>(client_info)
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
        &mut token,
    )
    .map_err(|_e| InjectorError::DebugPrivilegesFailed(format!("OpenThreadToken: {_e:?}")))?;

    let mut luid: LUID = Default::default();
    LookupPrivilegeValueW(None, SE_DEBUG_NAME, &mut luid).map_err(|_e| {
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
    let tid = GetWindowThreadProcessId(hwnd, Some(&mut pid));

    let mut clients = CLIENTS.lock().unwrap();

    if window_title == "World of Warcraft" {
        let index = clients.len();
        clients.push(WowClient {
            window_handle: hwnd,
            window_title,
            class_name,
            pid,
            tid,
            index: index as i32,
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
        res.push(client.inject(Some(account.clone()), &query)?);
        client.register_hotkey()?;
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
