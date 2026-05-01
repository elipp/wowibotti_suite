use shared::SendSyncWrapper;
use std::ffi::OsString;
use std::os::windows::ffi::OsStrExt;
use std::sync::OnceLock;
use windows::Win32::Foundation::{LPARAM, LRESULT, WPARAM};
use windows::Win32::UI::Input::KeyboardAndMouse::{MOD_ALT, RegisterHotKey, UnregisterHotKey};
use windows::Win32::UI::WindowsAndMessaging::{
    CS_HREDRAW, CS_VREDRAW, CW_USEDEFAULT, CreateWindowExW, DefWindowProcW, RegisterClassW,
    SetForegroundWindow, WINDOW_EX_STYLE, WNDCLASSW, WS_OVERLAPPEDWINDOW,
};
use windows::core::w;
use windows::{
    Win32::{
        Foundation::{FALSE, HWND},
        System::LibraryLoader::GetModuleHandleW,
        UI::WindowsAndMessaging::{
            DispatchMessageW, GetMessageW, MSG, TranslateMessage, WM_HOTKEY,
        },
    },
    core::PCWSTR,
};

use crate::windows::inject::{
    CLIENTS, INJ_MESSAGE_REGISTER_HOTKEY, INJ_MESSAGE_UNREGISTER_HOTKEY, InjectorError,
};

pub static DUMMY_WINDOW_HWND: OnceLock<SendSyncWrapper<HWND>> = OnceLock::new();

pub unsafe extern "system" fn dummy_wndproc(
    hwnd: HWND,
    msg: u32,
    wparam: WPARAM,
    lparam: LPARAM,
) -> LRESULT {
    unsafe {
        match msg {
            WM_HOTKEY => {
                let clients = CLIENTS.lock().unwrap();
                if let Some(client) = clients.get(wparam.0) {
                    if SetForegroundWindow(client.0.window_handle) == FALSE {
                        tracing::error!("warning: `SetForegroundWindow` failed");
                    }
                }
            }
            INJ_MESSAGE_REGISTER_HOTKEY => {
                if let Err(e) =
                    RegisterHotKey(Some(hwnd), wparam.0 as i32, MOD_ALT, lparam.0 as u32)
                {
                    tracing::error!("Warning: RegisterHotKey failed: {e:?}");
                }
            }

            INJ_MESSAGE_UNREGISTER_HOTKEY => {
                if let Err(e) = UnregisterHotKey(Some(hwnd), wparam.0 as i32) {
                    tracing::error!("Warning: UnregisterHotKey failed: {e:?}");
                }
            }
            _ => {}
        }
        DefWindowProcW(hwnd, msg, wparam, lparam)
    }
}

pub fn start_dummy_window() -> std::thread::JoinHandle<Result<(), InjectorError>> {
    std::thread::spawn(|| unsafe {
        let instance = GetModuleHandleW(None).unwrap();

        let class_name_buf: Vec<u16> = "InjectorHiddenWindow\0".encode_utf16().collect();
        let class_name = PCWSTR(class_name_buf.as_ptr());

        let window_class = WNDCLASSW {
            style: CS_HREDRAW | CS_VREDRAW,
            lpfnWndProc: Some(dummy_wndproc),
            hInstance: instance.into(),
            lpszClassName: class_name,
            ..Default::default()
        };

        RegisterClassW(&window_class);

        match CreateWindowExW(
            WINDOW_EX_STYLE(0),
            class_name,
            w!("injector"),
            WS_OVERLAPPEDWINDOW, //| WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            None,
            None,
            None, // used to be instance
            None,
        ) {
            Ok(hwnd) => {
                tracing::info!("hidden dummy window created");
                DUMMY_WINDOW_HWND.get_or_init(|| SendSyncWrapper(hwnd));
            }
            Err(_e) => {
                tracing::error!("{_e:?}");
                return Err(InjectorError::OtherError(format!("CreateWindowExW failed")));
            }
        }

        // Normally you would show the window with ShowWindow, but we'll keep it hidden.
        // ShowWindow(hwnd, SW_SHOW);

        tracing::info!("dummy window listening for events");
        let mut msg = MSG::default();
        while GetMessageW(&mut msg, None, 0, 0) != FALSE {
            let _ = TranslateMessage(&msg);
            DispatchMessageW(&msg);
            tracing::debug!("{msg:?}");
        }
        tracing::info!("dummy window exiting");
        Ok(())
    })
}

pub fn str_into_vec_u16<S: Into<OsString>>(s: S) -> Vec<u16> {
    let oss = s.into();
    oss.as_os_str().encode_wide().chain([0u16]).collect()
}

pub mod inject {
    use serde::Serialize;
    use shared::SendSyncWrapper;
    use std::sync::{Arc, LazyLock, Mutex};
    use windows::Win32::Foundation::WPARAM;
    use windows::Win32::UI::WindowsAndMessaging::{SendMessageW, WM_USER};
    use windows::core::BOOL;
    use wowibottihookdll::get_config_file_path;

    use windows::{
        Win32::{
            Foundation::{HANDLE, HWND, LPARAM, LUID},
            Security::{
                AdjustTokenPrivileges, ImpersonateSelf, LookupPrivilegeValueW, SE_DEBUG_NAME,
                SE_PRIVILEGE_ENABLED, SecurityImpersonation, TOKEN_ADJUST_PRIVILEGES,
                TOKEN_PRIVILEGES, TOKEN_QUERY,
            },
            System::Threading::{GetCurrentThread, OpenThreadToken},
            UI::WindowsAndMessaging::{
                EnumWindows, GetClassNameW, GetWindowTextW, GetWindowThreadProcessId,
            },
        },
        core::PCWSTR,
    };

    use super::{DUMMY_WINDOW_HWND, str_into_vec_u16};
    use crate::types::PottiConfig;
    use crate::{InjectQuery, LaunchQuery};

    pub const INJ_MESSAGE_REGISTER_HOTKEY: u32 = WM_USER;
    pub const INJ_MESSAGE_UNREGISTER_HOTKEY: u32 = WM_USER + 1;

    pub static CLIENTS: LazyLock<Arc<Mutex<Vec<SendSyncWrapper<WowClient>>>>> =
        LazyLock::new(|| Arc::new(Mutex::new(Vec::new())));

    pub type InjectorResult<T> = Result<T, InjectorError>;

    #[derive(Debug, thiserror::Error)]
    pub enum InjectorError {
        #[error("Debug privileges failed: {0}")]
        DebugPrivilegesFailed(String),
        #[error("Serialization error: {0}")]
        SerializationError(String),
        #[error("Deserialization error: {0}")]
        DeserializationError(String),
        #[error("Injection error: {0}")]
        InjectionError(String),
        #[error("Not found")]
        NotFound,
        #[cfg(feature = "web")]
        #[error("HTTP error: {0}")]
        HttpError(http::Error),
        #[error("Character entry not found: {0}")]
        CharacterEntryNotFound(String),
        #[error("Launch error: {0}")]
        LaunchError(String),
        #[error("Hotkey error: {0}")]
        HotkeyError(String),
        #[error("Other error: {0}")]
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

    impl LaunchQuery {
        #[cfg(feature = "host-windows")]
        pub fn launch_all(self, config: &PottiConfig) -> anyhow::Result<()> {
            let path = config.wow_client_path.clone().into_os_string();
            // let as_u16: Vec<u16> = str_into_vec_u16(path.clone());
            for config in self.configs.iter() {
                let id = config.id.to_string();
                let config_file_path = get_config_file_path(&id)?;
                config.write_to_file(&config_file_path)?;

                // let env = Self::get_env_with_lole_id(&id);

                let _ = std::process::Command::new(&path)
                    .env("LOLE_ID", id)
                    .spawn()?;

                std::thread::sleep(std::time::Duration::from_millis(200));
            }
            Ok(())
        }
    }

    impl WowClient {
        fn register_hotkey(&self) -> anyhow::Result<()> {
            let index_str = str_into_vec_u16((self.index + 1).rem_euclid(10).to_string());
            if index_str.len() < 1 {
                return Err(InjectorError::HotkeyError(format!(
                    "Hotkey vkeycode conversion failed"
                )))?;
            }
            tracing::info!("Registering hotkey for {self:?}");
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
                )))?
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

        // fn inject(
        //     &self,
        //     realm_info: Option<RealmInfo>,
        //     account: Option<Account>,
        //     query: &InjectQuery,
        // ) -> InjectorResult<WowClientInfo> {
        //     let pid = self.pid;
        //     let client_info = self.into();
        //     let enabled_patches = query.enabled_patches.clone();
        //     let modified_client = std::thread::spawn(move || unsafe {
        //         obtain_debug_privileges()?;

        //         if let Some(account) = account {
        //             account
        //                 .write_config_to_tmp_file(pid, realm_info.as_ref(), enabled_patches)
        //                 .map_err(|s| InjectorError::OtherError(s))?;
        //         }

        //         let process = OpenProcess(PROCESS_ALL_ACCESS, false, pid).unwrap();

        //         let load_library_addr = GetProcAddress(
        //             GetModuleHandleW(w!("kernel32.dll")).map_err(|_e| {
        //                 InjectorError::InjectionError(String::from("GetModuleHandleW"))
        //             })?,
        //             s!("LoadLibraryW"),
        //         )
        //         .ok_or_else(|| InjectorError::InjectionError(String::from("GetProcAddress")))?;

        //         let dll_path = std::env::current_dir().unwrap().join("lolerust.dll");

        //         if !dll_path.exists() {
        //             return Err(InjectorError::InjectionError(format!(
        //                 "dll file `{}` doesn't exist",
        //                 dll_path.display()
        //             )));
        //         }

        //         let as_u16: Vec<u16> = str_into_vec_u16(dll_path);
        //         let path_len_bytes = as_u16.len() * std::mem::size_of::<i16>();

        //         let llparam = VirtualAllocEx(
        //             process,
        //             None,
        //             path_len_bytes,
        //             MEM_RESERVE | MEM_COMMIT,
        //             PAGE_READWRITE,
        //         );

        //         if llparam.is_null() {
        //             return Err(InjectorError::InjectionError(String::from(
        //                 "VirtualAllocEx",
        //             )));
        //         }

        //         WriteProcessMemory(
        //             process,
        //             llparam,
        //             as_u16.as_ptr() as *const c_void,
        //             path_len_bytes,
        //             None,
        //         )
        //         .map_err(|_e| InjectorError::InjectionError(String::from("WriteProcessMemory")))?;

        //         let load_library_addr = std::mem::transmute::<
        //             _,
        //             unsafe extern "system" fn(*mut c_void) -> u32,
        //         >(load_library_addr as *const ());

        //         let rt = CreateRemoteThread(
        //             process,
        //             None,
        //             0,
        //             Some(load_library_addr),
        //             Some(llparam),
        //             0,
        //             None,
        //         )
        //         .map_err(|_e| InjectorError::InjectionError(String::from("CreateRemoteThread")))?;

        //         WaitForSingleObject(rt, INFINITE);
        //         let mut library_handle = 0u32;
        //         GetExitCodeThread(rt, &mut library_handle).map_err(|_e| {
        //             InjectorError::InjectionError(String::from("GetExitCodeThread"))
        //         })?;
        //         CloseHandle(process).unwrap();
        //         if library_handle == 0 {
        //             Err(InjectorError::InjectionError(String::from("LoadLibraryW")))
        //         } else {
        //             Ok::<WowClientInfo, InjectorError>(client_info)
        //         }
        //     })
        //     .join()
        //     .map_err(|e| InjectorError::InjectionError(format!("{e:?}")))?
        //     .map_err(|e| InjectorError::InjectionError(format!("{e:?}")))?;
        //     Ok(modified_client)
        // }
    }

    unsafe fn obtain_debug_privileges() -> anyhow::Result<()> {
        let mut token: HANDLE = Default::default();
        unsafe {
            ImpersonateSelf(SecurityImpersonation)?;
            OpenThreadToken(
                GetCurrentThread(),
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                false,
                &mut token,
            )?;

            let mut luid: LUID = Default::default();
            LookupPrivilegeValueW(None, SE_DEBUG_NAME, &mut luid)?;

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
            )?;
        }

        return Ok(());
    }

    unsafe extern "system" fn enum_windows_proc(hwnd: HWND, _lparam: LPARAM) -> BOOL {
        unsafe {
            let mut buf = vec![0u16; 1024];
            if GetClassNameW(hwnd, &mut buf) == 0 {
                return true.into();
            }
            let class_name = PCWSTR::from_raw(buf.as_ptr())
                .to_string()
                .unwrap_or_default();

            buf.fill(0); // clear before reuse
            GetWindowTextW(hwnd, &mut buf);
            let window_title = PCWSTR::from_raw(buf.as_ptr())
                .to_string()
                .unwrap_or_default();

            let mut clients = CLIENTS.lock().unwrap();

            if window_title == "World of Warcraft" {
                let index = clients.len();

                let mut pid = 0u32;
                let tid = GetWindowThreadProcessId(hwnd, Some(&mut pid));

                clients.push(SendSyncWrapper(WowClient {
                    window_handle: hwnd,
                    window_title,
                    class_name,
                    pid,
                    tid,
                    index: index as i32,
                    library_handle: None,
                }));
            }
        }

        true.into()
    }

    fn inject_dll(
        clients: &Vec<WowClient>,
        config: PottiConfig,
        query: InjectQuery,
    ) -> InjectorResult<Vec<WowClientInfo>> {
        let mut res = Vec::default();
        // for (client, name) in clients.iter().zip(query.enabled_characters.iter()) {
        //     let account = config
        //         .accounts
        //         .iter()
        //         .find(|a| &a.character.name == name)
        //         .ok_or_else(|| InjectorError::CharacterEntryNotFound(name.to_owned()))?;
        //     res.push(client.inject(
        //         config.realm.clone(),
        //         Some(Account(account.clone())),
        //         &query,
        //     )?);
        //     client.register_hotkey()?;
        // }
        Ok(res)
    }

    fn enum_windows() {
        CLIENTS.lock().unwrap().clear();
        unsafe { EnumWindows(Some(enum_windows_proc), LPARAM::default()) }.unwrap();
    }

    pub fn find_wow_windows_and_register_hotkeys() -> anyhow::Result<()> {
        enum_windows();
        let clients = CLIENTS.lock().unwrap();
        for client in clients.iter() {
            client.0.register_hotkey()?;
        }
        Ok(())
    }
}
