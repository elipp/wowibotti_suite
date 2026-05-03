#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")] // hide console window on Windows in release
#![allow(rustdoc::missing_crate_level_docs)]

use anyhow::Context;
#[cfg(feature = "native-ui")]
use eframe::egui;

use tracing_subscriber::{layer::SubscriberExt, util::SubscriberInitExt};

use serde::Deserialize;
use std::path::Path;
use uuid::Uuid;

use wowibotti_suite_types::ClientConfig;

#[cfg(feature = "web")]
mod tokio_io;
#[cfg(feature = "web")]
use crate::tokio_io::TokioIo;
use crate::types::Account;

#[cfg(feature = "web")]
mod web;

#[cfg(feature = "host-windows")]
mod windows;

#[cfg(feature = "host-linux")]
mod linux;

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
    pub configs: Vec<ClientConfig>,
}

struct Togglable<T> {
    value: T,
    enabled: bool,
}

struct InjectorApp {
    select_all: bool,
    accounts: Vec<Togglable<Account>>,
    patches: Vec<Togglable<PatchConfig>>,
    config: PottiConfig,
    enable_addonmessage_broker_client: bool,
    addonmessage_broker_server_started: bool,
}

impl InjectorApp {
    fn enabled_patches(&self) -> Vec<String> {
        self.patches
            .iter()
            .filter(|t| t.enabled)
            .map(|t| t.value.name.clone())
            .collect()
    }
    fn enabled_characters(&self) -> Vec<String> {
        self.accounts
            .iter()
            .filter(|t| t.enabled)
            .map(|t| t.value.0.character.name.clone())
            .collect()
    }
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
                        let checkbox = ui
                            .checkbox(&mut account.enabled, account.value.0.character.name.clone());
                        if checkbox.changed() {
                            self.select_all = false;
                        }
                        ui.image(format!(
                            "file://injector/assets/{}.png",
                            account.value.0.character.class
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
            ui.add_space(10.0);
            ui.horizontal(|ui| {
                let checkbox_enabled = self.config.addonmessage_broker_addr.is_some();
                ui.add_enabled_ui(checkbox_enabled, |ui| {
                    ui.checkbox(
                        &mut self.enable_addonmessage_broker_client,
                        if checkbox_enabled {
                            format!(
                                "Enable addonmessage broker client @ {}",
                                self.config.addonmessage_broker_addr.as_ref().unwrap()
                            )
                        } else {
                            "Enable addonmessage broker (needs potti.conf configuration)".into()
                        },
                    );
                })
            });
            ui.add_space(50.0);
            ui.horizontal(|ui| {
                let launch_button_enabled = !self.enabled_characters().is_empty();
                ui.add_enabled_ui(launch_button_enabled, |ui| {
                    if ui.button("Launch clients").clicked() {
                        let query = LaunchQuery {
                            configs: self
                                .accounts
                                .iter()
                                .filter(|t| t.enabled)
                                .map(|a| ClientConfig {
                                    realm: self.config.realm.clone(),
                                    account: Some(a.value.0.clone()),
                                    enabled_patches: self.enabled_patches(),
                                    log_level: None,
                                    id: Uuid::new_v4(),
                                    path_override: a.value.0.path_override.clone(),
                                    addonmessage_broker_addr: if self
                                        .enable_addonmessage_broker_client
                                    {
                                        self.config.addonmessage_broker_addr.clone()
                                    } else {
                                        None
                                    },
                                })
                                .collect(),
                        };
                        query.launch_all(&self.config).unwrap();
                    }
                });

                ui.add_enabled_ui(!self.addonmessage_broker_server_started, |ui| {
                    if ui.button("Start addonmessage broker server").clicked() {
                        start_addonmessage_broker_server();
                        self.addonmessage_broker_server_started = true;
                    }
                });

                #[cfg(feature = "host-linux")]
                if ui.button("Assign windows (niri)").clicked() {
                    match std::process::Command::new("/bin/bash")
                        .arg(format!(
                            "{}/randomize_windows_niri.sh",
                            std::env!("CARGO_MANIFEST_DIR")
                        ))
                        .spawn()
                    {
                        Ok(_) => {}
                        Err(e) => {
                            tracing::error!("Command failed {e:?}");
                        }
                    }
                }
                #[cfg(feature = "host-windows")]
                if ui.button("Assign hotkeys").clicked() {
                    use crate::windows::inject::find_wow_windows_and_register_hotkeys;
                    match find_wow_windows_and_register_hotkeys() {
                        Ok(_) => {}
                        Err(e) => tracing::error!("hotkeys: {e}"),
                    }
                }
            });
        });
    }
}

#[cfg(feature = "addonmessage_broker")]
fn start_addonmessage_broker_server() {
    let _ = std::thread::spawn(|| {
        let rt = tokio::runtime::Runtime::new().unwrap();
        rt.block_on(async {
            start_addonmessage_relay().await;
        })
    });
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

    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size([400.0, 300.0])
            .with_app_id("lole"),
        ..Default::default()
    };

    let config = read_potti_conf().unwrap();

    #[cfg(feature = "host-windows")]
    {
        // Windows needs this to configure hotkeys
        use crate::windows::start_dummy_window;
        start_dummy_window();
    }

    let accounts: Vec<_> = config
        .accounts
        .iter()
        .map(|a| Togglable {
            value: Account(a.clone()),
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

    eframe::run_native(
        "injector :D",
        options,
        Box::new(move |cc| {
            egui_extras::install_image_loaders(&cc.egui_ctx);
            Ok(Box::new(InjectorApp {
                select_all: false,
                accounts,
                patches,
                config,
                enable_addonmessage_broker_client: false,
                addonmessage_broker_server_started: false,
            }))
        }),
    )
}

#[cfg(feature = "web")]
#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    web::main().await
}
