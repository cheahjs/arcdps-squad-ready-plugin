use anyhow::{Context, Result};
use log::info;
use serde::{Deserialize, Serialize};
use std::fs;
use std::path::PathBuf;

const SETTINGS_FILE: &str = "addons/arcdps/arcdps_squad_ready.json";

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Settings {
    #[serde(default)]
    pub version: u32,

    #[serde(default)]
    pub ready_check_path: Option<String>,

    #[serde(default)]
    pub squad_ready_path: Option<String>,

    #[serde(default = "default_volume")]
    pub ready_check_volume: i32,

    #[serde(default = "default_volume")]
    pub squad_ready_volume: i32,

    #[serde(default = "default_true")]
    pub flash_window: bool,

    #[serde(default)]
    pub ready_check_nag: bool,

    #[serde(default)]
    pub ready_check_nag_in_combat: bool,

    #[serde(default = "default_nag_interval")]
    pub ready_check_nag_interval_seconds: f32,

    #[serde(default)]
    pub audio_output_device: Option<String>,

    #[serde(default = "default_true")]
    pub check_for_updates: bool,

    #[serde(default)]
    pub include_prereleases: bool,
}

fn default_volume() -> i32 {
    100
}

fn default_true() -> bool {
    true
}

fn default_nag_interval() -> f32 {
    5.0
}

impl Default for Settings {
    fn default() -> Self {
        Self {
            version: 1,
            ready_check_path: None,
            squad_ready_path: None,
            ready_check_volume: 100,
            squad_ready_volume: 100,
            flash_window: true,
            ready_check_nag: false,
            ready_check_nag_in_combat: false,
            ready_check_nag_interval_seconds: 5.0,
            audio_output_device: None,
            check_for_updates: true,
            include_prereleases: false,
        }
    }
}

impl Settings {
    /// Load settings from the JSON file.
    pub fn load() -> Result<Self> {
        let path = Self::settings_path();

        if !path.exists() {
            info!("settings file not found, using defaults");
            return Ok(Self::default());
        }

        let content = fs::read_to_string(&path)
            .with_context(|| format!("failed to read settings file: {:?}", path))?;

        let settings: Settings =
            serde_json::from_str(&content).with_context(|| "failed to parse settings JSON")?;

        Ok(settings)
    }

    /// Save settings to the JSON file.
    pub fn save(&self) -> Result<()> {
        let path = Self::settings_path();

        // Ensure parent directory exists
        if let Some(parent) = path.parent() {
            fs::create_dir_all(parent)
                .with_context(|| format!("failed to create settings directory: {:?}", parent))?;
        }

        let content =
            serde_json::to_string_pretty(self).with_context(|| "failed to serialize settings")?;

        fs::write(&path, content)
            .with_context(|| format!("failed to write settings file: {:?}", path))?;

        info!("settings saved to {:?}", path);
        Ok(())
    }

    fn settings_path() -> PathBuf {
        PathBuf::from(SETTINGS_FILE)
    }
}
