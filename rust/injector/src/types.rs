use std::path::PathBuf;

use serde::{Deserialize, Serialize};
use wowibotti_suite_types::{CharacterInfo, RealmInfo, WowAccount};

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
    pub realm: Option<RealmInfo>,
    pub log_level: Option<String>,
}

#[derive(Debug)]
pub(crate) struct Account(pub(crate) WowAccount);

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
