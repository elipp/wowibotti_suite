#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")] // hide console window on Windows in release
#![allow(rustdoc::missing_crate_level_docs)]

use anyhow::Context;
#[cfg(feature = "native-ui")]
use eframe::egui;

use tracing_subscriber::{layer::SubscriberExt, util::SubscriberInitExt};

use serde::Deserialize;
use std::ffi::OsString;
use std::path::Path;

#[cfg(feature = "host-windows")]
use os::windows::ffi::OsStrExt;

use wowibotti_suite_types::WowAccount;

#[cfg(feature = "web")]
mod tokio_io;
#[cfg(feature = "web")]
use crate::tokio_io::TokioIo;
#[cfg(feature = "web")]
mod web;

#[cfg(feature = "host-windows")]
mod windows;

mod types;

use types::{PatchConfig, PottiConfig};

#[cfg(feature = "addonmessage_broker")]
use addonmessage_broker::server::start_addonmessage_relay;

#[cfg(all(not(feature = "native-ui"), not(feature = "web")))]
compile_error!("one of `--feature=native-ui` or `--feature=web` must be provided");

#[derive(Debug, Deserialize)]
pub struct InjectQuery {
    pub enabled_characters: Vec<String>,
    pub enabled_patches: Vec<String>,
}

const CONFIG_FILENAME: &str = "potti.conf.json";

pub fn read_potti_conf() -> anyhow::Result<PottiConfig> {
    let path = Path::new(std::env!("CARGO_MANIFEST_PATH"))
        .parent()
        .context("CARGO_MANIFEST_PATH error")?
        .join(CONFIG_FILENAME);
    let config_str = std::fs::read_to_string(&path)?;
    let config: PottiConfig = serde_json::from_str(&config_str)?;
    Ok(config)
}

#[derive(Debug, Deserialize)]
pub struct LaunchQuery {
    pub num_clients: i32,
}

impl LaunchQuery {
    #[cfg(feature = "host-windows")]
    fn launch(self, config: PottiConfig) -> anyhow::Result<()> {
        unsafe {
            let path = config.wow_client_path.into_os_string();
            let as_u16: Vec<u16> = str_into_vec_u16(path.clone());
            for _ in 0..self.num_clients {
                let startup_info = STARTUPINFOW::default();
                let mut process_information = PROCESS_INFORMATION::default();
                CreateProcessW(
                    PCWSTR::from_raw(as_u16.as_ptr()),
                    None,
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

    #[cfg(feature = "host-linux")]
    fn launch(self, config: PottiConfig) -> anyhow::Result<()> {
        let path = config.wow_client_path;
        for _ in 0..self.num_clients {
            let _ = std::process::Command::new("wine").arg(&path).spawn()?;
            std::thread::sleep(std::time::Duration::from_millis(200));
        }
        Ok(())
    }
}

struct Togglable<T> {
    value: T,
    enabled: bool,
}

struct InjectorApp {
    select_all: bool,
    accounts: Vec<Togglable<WowAccount>>,
    patches: Vec<Togglable<PatchConfig>>,
    config: PottiConfig,
}

impl eframe::App for InjectorApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        egui::CentralPanel::default().show(ctx, |ui| {
            ui.heading("Injector :D");
            ui.add_space(20.0);
            ui.columns(2, |columns| {
                columns[0].label("Available characters:");
                egui::Grid::new("character_list").show(&mut columns[0], |ui| {
                    let toggle = ui.toggle_value(&mut self.select_all, "Select all");
                    if toggle.changed() {
                        for account in self.accounts.iter_mut() {
                            account.enabled = self.select_all;
                        }
                    }
                    ui.end_row();
                    for account in self.accounts.iter_mut() {
                        let checkbox =
                            ui.checkbox(&mut account.enabled, account.value.character.name.clone());
                        if checkbox.changed() {
                            self.select_all = false;
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
                    for patch in self.patches.iter_mut() {
                        ui.checkbox(&mut patch.enabled, patch.value.name.clone());
                        ui.end_row();
                    }
                });
            });
            ui.add_space(50.0);
            ui.horizontal(|ui| {
                #[cfg(feature = "host-windows")]
                if ui.button("Inject DLL").clicked() {
                    find_wow_windows_and_inject(
                        self.config.clone(),
                        InjectQuery {
                            enabled_characters: self
                                .accounts
                                .iter()
                                .filter(|t| t.enabled)
                                .map(|t| t.value.character.name.clone())
                                .collect(),
                            enabled_patches: self
                                .patches
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
                        num_clients: self.accounts.iter().filter(|t| t.enabled).count() as i32,
                    };
                    query.launch(self.config.clone()).unwrap();
                }
            });
        });
    }
}

#[cfg(feature = "native-ui")]
fn main() -> Result<(), eframe::Error> {
    tracing_subscriber::registry()
        .with(
            tracing_subscriber::EnvFilter::try_from_default_env()
                .unwrap_or_else(|_| tracing_subscriber::EnvFilter::new("info")),
        )
        .with(tracing_subscriber::fmt::layer().with_ansi(true))
        .init();

    #[cfg(feature = "host-windows")]
    start_dummy_window();
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size([400.0, 300.0])
            .with_app_id("lole"),
        ..Default::default()
    };

    let config = read_potti_conf().unwrap();
    let accounts: Vec<_> = config
        .accounts
        .iter()
        .map(|a| Togglable {
            value: a.clone(),
            enabled: false,
        })
        .collect();
    let patches: Vec<_> = config
        .available_patches
        .iter()
        .map(|a| Togglable {
            value: a.clone(),
            enabled: a.enabled_by_default,
        })
        .collect();

    #[cfg(feature = "addonmessage_broker")]
    let _ = std::thread::spawn(|| {
        let rt = tokio::runtime::Runtime::new().unwrap();
        rt.block_on(async {
            start_addonmessage_relay().await;
        })
    });

    let select_all = false;

    eframe::run_native(
        "injector :D",
        options,
        Box::new(move |cc| {
            egui_extras::install_image_loaders(&cc.egui_ctx);
            Ok(Box::new(InjectorApp {
                select_all,
                accounts,
                patches,
                config,
            }))
        }),
    )
}

#[cfg(feature = "web")]
#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    web::main().await
}
