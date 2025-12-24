use anyhow::Result;
use arcdps::extras::ExtrasAddonInfo;
use arcdps::extras::UserInfoIter;
use arcdps::imgui::Ui;
use log::*;
use once_cell::sync::Lazy;
use std::sync::Mutex;

use crate::audio::player::AudioPlayer;
use crate::file_picker::FilePicker;
use crate::settings::Settings;
use crate::settings_ui;
use crate::squad_tracker::SquadTracker;

pub static AUDIO_PLAYER: Lazy<Mutex<AudioPlayer>> = Lazy::new(|| Mutex::new(AudioPlayer::new()));

pub struct Plugin {
    settings: Settings,
    squad_tracker: SquadTracker,
    extras_loaded: bool,
    self_account_name: String,
    ready_check_picker: FilePicker,
    squad_ready_picker: FilePicker,
}

impl Plugin {
    pub fn new() -> Self {
        Self {
            settings: Settings::default(),
            squad_tracker: SquadTracker::new(),
            extras_loaded: false,
            self_account_name: String::new(),
            ready_check_picker: FilePicker::new("Select Ready Check Sound".to_string(), None),
            squad_ready_picker: FilePicker::new("Select Squad Ready Sound".to_string(), None),
        }
    }

    pub fn load(&mut self) -> Result<()> {
        info!("loading Squad Ready plugin");

        // Load settings
        match Settings::load() {
            Ok(settings) => {
                self.settings = settings;
                info!("settings loaded successfully");
            }
            Err(err) => {
                warn!("failed to load settings, using defaults: {:#}", err);
            }
        }

        // Initialize audio
        self.init_audio();

        info!("Squad Ready plugin loaded");
        Ok(())
    }

    fn init_audio(&self) {
        let player = AUDIO_PLAYER.lock().unwrap();

        // Set device if configured
        if let Some(ref device) = self.settings.audio_output_device {
            player.set_device(Some(device.clone()));
        }
    }

    pub fn release(&mut self) {
        info!("releasing Squad Ready plugin");

        // Save settings
        if let Err(err) = self.settings.save() {
            error!("failed to save settings: {:#}", err);
        }

        // Release audio
        AUDIO_PLAYER.lock().unwrap().release();

        info!("Squad Ready plugin released");
    }

    pub fn extras_init(&mut self, _addon_info: &ExtrasAddonInfo, account_name: Option<&str>) {
        self.extras_loaded = true;
        if let Some(name) = account_name {
            let name = name.strip_prefix(':').unwrap_or(name);
            self.self_account_name = name.to_string();
            debug!("self account name: {}", self.self_account_name);
        }
    }

    pub fn squad_update(&mut self, users: UserInfoIter) {
        self.squad_tracker.update_users(
            users,
            &self.self_account_name,
            &self.settings,
        );
    }

    pub fn render_windows(&mut self, _ui: &Ui, _not_loading: bool) {
        // Tick the squad tracker (handles nag timer)
        self.squad_tracker.tick(&self.settings);

        // Render debug window if visible
        // (Currently not implemented - could add debug window here)
    }

    pub fn render_settings(&mut self, ui: &Ui) {
        settings_ui::draw(
            ui,
            &mut self.settings,
            &mut self.ready_check_picker,
            &mut self.squad_ready_picker,
            self.extras_loaded,
        );
    }
}
