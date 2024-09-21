#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")] // hide console window on Windows in release
#![allow(rustdoc::missing_crate_level_docs)]

use addonmessage_broker::SendSyncWrapper;
#[cfg(feature = "native-ui")]
use eframe::egui;

use lazy_static::lazy_static;
use serde::{Deserialize, Serialize};
use std::sync::OnceLock;
use std::{ffi::OsString, os::windows::ffi::OsStrExt, path::PathBuf};
use windows::core::w;
use windows::Win32::Foundation::{LPARAM, LRESULT, WPARAM};
use windows::Win32::UI::Input::KeyboardAndMouse::{RegisterHotKey, UnregisterHotKey, MOD_ALT};
use windows::Win32::UI::WindowsAndMessaging::{
    CreateWindowExW, DefWindowProcW, RegisterClassW, SetForegroundWindow, CS_HREDRAW, CS_VREDRAW,
    CW_USEDEFAULT, WINDOW_EX_STYLE, WM_USER, WNDCLASSW, WS_OVERLAPPEDWINDOW, WS_VISIBLE,
};
use windows::{
    core::PCWSTR,
    core::PWSTR,
    Win32::{
        Foundation::{FALSE, HWND},
        System::{
            LibraryLoader::GetModuleHandleW,
            Threading::{
                CreateProcessW, PROCESS_CREATION_FLAGS, PROCESS_INFORMATION, STARTUPINFOW,
            },
        },
        UI::WindowsAndMessaging::{
            DispatchMessageW, GetMessageW, TranslateMessage, MSG, WM_HOTKEY,
        },
    },
};

mod inject;

#[cfg(feature = "web")]
mod tokio_io;

#[cfg(feature = "web")]
use crate::tokio_io::TokioIo;

#[cfg(feature = "web")]
mod web;

use inject::{
    find_wow_windows_and_inject, InjectorError, InjectorResult, CLIENTS,
    INJ_MESSAGE_REGISTER_HOTKEY, INJ_MESSAGE_UNREGISTER_HOTKEY,
};

#[cfg(feature = "addonmessage_broker")]
use addonmessage_broker::server::start_addonmessage_relay;

use wowibottihookdll::{CharacterInfo, WowAccount};

pub static DUMMY_WINDOW_HWND: OnceLock<SendSyncWrapper<HWND>> = OnceLock::new();

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PatchConfig {
    pub name: String,
    pub enabled_by_default: bool,
}

#[derive(Debug, Clone, Deserialize)]
pub struct PottiConfig {
    pub wow_client_path: PathBuf,
    pub accounts: Vec<WowAccount>,
    pub available_patches: Vec<PatchConfig>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct ConfigResult {
    pub characters: Vec<CharacterInfo>,
    pub available_patches: Vec<PatchConfig>,
}

impl From<PottiConfig> for ConfigResult {
    fn from(p: PottiConfig) -> Self {
        Self {
            characters: p.accounts.into_iter().map(|a| a.character).collect(),
            available_patches: p.available_patches.clone(),
        }
    }
}

#[cfg(all(not(feature = "native-ui"), not(feature = "web")))]
compile_error!("one of `--feature=native-ui` or `--feature=web` must be provided");

#[derive(Debug, Deserialize)]
pub struct InjectQuery {
    pub enabled_characters: Vec<String>,
    pub enabled_patches: Vec<String>,
}

pub fn read_potti_conf() -> Result<PottiConfig, InjectorError> {
    // let config_str = std::fs::read_to_string("potti.conf")
    //     .map_err(|_e| InjectorError::OtherError(format!("couldn't read potti.conf")))?;
    let config_str = include_str!("..\\potti.conf.json");
    let config: PottiConfig = serde_json::from_str(&config_str)
        .map_err(|_e| InjectorError::DeserializationError(format! {"{_e:?}"}))?;
    Ok(config)
}

#[derive(Debug, Deserialize)]
pub struct LaunchQuery {
    pub num_clients: i32,
}

pub fn str_into_vec_u16<S: Into<OsString>>(s: S) -> Vec<u16> {
    let oss = s.into();
    oss.as_os_str().encode_wide().chain([0u16]).collect()
}

impl LaunchQuery {
    fn launch(self, config: PottiConfig) -> InjectorResult<()> {
        unsafe {
            let path = config.wow_client_path.into_os_string();
            let as_u16: Vec<u16> = str_into_vec_u16(path.clone());
            for _ in 0..self.num_clients {
                let startup_info = STARTUPINFOW::default();
                let mut process_information = PROCESS_INFORMATION::default();
                CreateProcessW(
                    PCWSTR::from_raw(as_u16.as_ptr()),
                    PWSTR::null(),
                    None,
                    None,
                    false,
                    PROCESS_CREATION_FLAGS(0),
                    None,
                    None,
                    &startup_info as *const STARTUPINFOW,
                    &mut process_information,
                )
                .map_err(|_e| InjectorError::LaunchError(format!("{_e:?} ({:?})", path)))?;
                std::thread::sleep(std::time::Duration::from_millis(200));
            }
        }
        Ok(())
    }
}

unsafe extern "system" fn dummy_wndproc(
    hwnd: HWND,
    msg: u32,
    wparam: WPARAM,
    lparam: LPARAM,
) -> LRESULT {
    match msg {
        WM_HOTKEY => {
            let clients = CLIENTS.lock().unwrap();
            if let Some(client) = clients.get(wparam.0) {
                if SetForegroundWindow(client.window_handle) == FALSE {
                    println!("warning: `SetForegroundWindow` failed");
                }
            }
        }
        INJ_MESSAGE_REGISTER_HOTKEY => {
            if let Err(e) = RegisterHotKey(hwnd, wparam.0 as i32, MOD_ALT, lparam.0 as u32) {
                println!("Warning: RegisterHotKey failed: {e:?}");
            }
        }

        INJ_MESSAGE_UNREGISTER_HOTKEY => {
            if let Err(e) = UnregisterHotKey(hwnd, wparam.0 as i32) {
                println!("Warning: UnregisterHotKey failed: {e:?}");
            }
        }
        _ => {}
    }
    DefWindowProcW(hwnd, msg, wparam, lparam)
}

pub fn start_dummy_window() -> std::thread::JoinHandle<Result<(), InjectorError>> {
    std::thread::spawn(|| unsafe {
        let instance = GetModuleHandleW(None).unwrap();

        let class_name = PCWSTR(
            "InjectorHiddenWindow"
                .encode_utf16()
                .collect::<Vec<u16>>()
                .as_ptr(),
        );
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
            instance,
            None,
        ) {
            Ok(hwnd) => {
                DUMMY_WINDOW_HWND.get_or_init(|| SendSyncWrapper(hwnd));
            }
            Err(_e) => {
                return Err(InjectorError::OtherError(format!("CreateWindowExW failed")));
            }
        }

        // Normally you would show the window with ShowWindow, but we'll keep it hidden.
        // ShowWindow(hwnd, SW_SHOW);

        let mut msg = MSG::default();
        while GetMessageW(&mut msg, None, 0, 0) != FALSE {
            let _ = TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        println!("exiting");
        Ok(())
    })
}

struct Togglable<T> {
    value: T,
    enabled: bool,
}

#[cfg(feature = "native-ui")]
fn main() -> Result<(), eframe::Error> {
    // env_logger::init(); // Log to stderr (if you run with `RUST_LOG=debug`).
    start_dummy_window();
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default().with_inner_size([400.0, 300.0]),
        ..Default::default()
    };

    let config = read_potti_conf().unwrap();
    let mut accounts: Vec<_> = config
        .accounts
        .iter()
        .map(|a| Togglable {
            value: a.clone(),
            enabled: false,
        })
        .collect();
    let mut patches: Vec<_> = config
        .available_patches
        .iter()
        .map(|a| Togglable {
            value: a.clone(),
            enabled: a.enabled_by_default,
        })
        .collect();

    #[cfg(feature = "addonmessage_broker")]
    let handle = std::thread::spawn(|| {
        let rt = tokio::runtime::Runtime::new().unwrap();
        rt.block_on(async {
            start_addonmessage_relay().await;
        })
    });

    let mut select_all = false;

    eframe::run_simple_native("injector :D", options, move |ctx, _frame| {
        egui_extras::install_image_loaders(ctx);
        egui::CentralPanel::default().show(ctx, |ui| {
            ui.heading("Injector :D");
            ui.add_space(20.0);
            ui.columns(2, |columns| {
                columns[0].label("Available characters:");
                egui::Grid::new("character_list").show(&mut columns[0], |ui| {
                    let toggle = ui.toggle_value(&mut select_all, "Select all");
                    if toggle.changed() {
                        for account in accounts.iter_mut() {
                            account.enabled = select_all;
                        }
                    }
                    ui.end_row();

                    for account in accounts.iter_mut() {
                        let checkbox =
                            ui.checkbox(&mut account.enabled, account.value.character.name.clone());
                        if checkbox.changed() {
                            select_all = false;
                        }
                        ui.image(format!(
                            "file://injector/assets/{}.png",
                            account.value.character.class
                        ));
                        ui.end_row();
                    }
                });
                columns[1].label("Patches to enable:");
                egui::Grid::new("enabled_patches").show(&mut columns[1], |ui| {
                    for patch in patches.iter_mut() {
                        ui.checkbox(&mut patch.enabled, patch.value.name.clone());
                        ui.end_row();
                    }
                });
            });
            ui.add_space(50.0);
            ui.horizontal(|ui| {
                if ui.button("Inject DLL").clicked() {
                    find_wow_windows_and_inject(
                        config.clone(),
                        InjectQuery {
                            enabled_characters: accounts
                                .iter()
                                .filter(|t| t.enabled)
                                .map(|t| t.value.character.name.clone())
                                .collect(),
                            enabled_patches: patches
                                .iter()
                                .filter(|t| t.enabled)
                                .map(|t| t.value.name.clone())
                                .collect(),
                        },
                    )
                    .unwrap();
                }

                if ui.button("Launch clients").clicked() {
                    let query = LaunchQuery {
                        num_clients: accounts
                            .iter()
                            .filter(|t| t.enabled)
                            .map(|t| t.value.character.name.clone())
                            .count() as i32,
                    };
                    query.launch(config.clone()).unwrap();
                }
            })
        });
    })
}

#[cfg(feature = "web")]
#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    web::main().await
}
