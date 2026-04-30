use std::path::Path;

use serde::{Deserialize, Serialize};
use uuid::Uuid;

#[derive(Debug, Clone, Copy, Serialize, Deserialize, Hash)]
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
#[derive(Debug, Clone, Serialize, Deserialize, Hash)]
pub struct CharacterInfo {
    pub name: String,
    pub class: CharacterClass,
}

#[derive(Debug, Clone, Serialize, Deserialize, Hash)]
pub struct WowAccount {
    pub username: String,
    pub password: String,
    pub character: CharacterInfo,
    pub path_override: Option<String>,
}

impl WowAccount {
    pub fn account_hash(&self) -> String {
        use std::collections::hash_map::DefaultHasher;
        use std::hash::{Hash, Hasher};

        let mut hasher = DefaultHasher::new();
        self.hash(&mut hasher);
        format!("{:x}", hasher.finish()) // hex string
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct RealmInfo {
    pub login_server: String,
    pub name: String,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct ClientConfig {
    pub realm: Option<RealmInfo>,
    pub account: Option<WowAccount>,
    pub enabled_patches: Vec<String>,
    pub log_level: Option<String>,
    pub id: Uuid,
    pub path_override: Option<String>,
}

impl ClientConfig {
    pub fn write_to_file(&self, path: impl AsRef<Path>) -> anyhow::Result<()> {
        let serialized = serde_json::to_string(&self)?;
        Ok(std::fs::write(&path, &serialized)?)
    }
}
