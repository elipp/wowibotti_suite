
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
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
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CharacterInfo {
    pub name: String,
    pub class: CharacterClass,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct WowAccount {
    pub username: String,
    pub password: String,
    pub character: CharacterInfo,
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
}
