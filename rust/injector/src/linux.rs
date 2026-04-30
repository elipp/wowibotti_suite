use std::path::{Path, PathBuf};

use crate::LaunchQuery;
use crate::types::PottiConfig;

pub fn get_config_file_path(identifier: &str) -> anyhow::Result<PathBuf> {
    Ok(
        Path::new("/home/elias/.wine/drive_c/users/elias/AppData/Local/Temp")
            .join(format!("wow-{identifier}.json")),
    )
}

impl LaunchQuery {
    #[cfg(feature = "host-linux")]
    pub fn launch_all(self, config: &PottiConfig) -> anyhow::Result<()> {
        for client_config in self.configs {
            let id = client_config.id.to_string();
            let config_file_path = get_config_file_path(&id)?;
            client_config.write_to_file(&config_file_path)?;

            // let _ = std::process::Command::new("winedbg")
            let _ = std::process::Command::new("wine")
                .arg(
                    client_config
                        .path_override
                        .as_ref()
                        .map(|p| Path::new(p))
                        .unwrap_or(&config.wow_client_path),
                )
                .env("LOLE_ID", id)
                .spawn()?;

            std::thread::sleep(std::time::Duration::from_millis(200));
        }
        Ok(())
    }
}
