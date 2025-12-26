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
    #[cfg(debug_assertions)]
    debug_window_visible: bool,
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
            #[cfg(debug_assertions)]
            debug_window_visible: true,
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

        // Release audio - wait for audio thread to terminate
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
        self.squad_tracker
            .update_users(users, &self.self_account_name, &self.settings);
    }

    #[cfg_attr(not(debug_assertions), allow(unused_variables))]
    pub fn render_windows(&mut self, ui: &Ui, _not_loading: bool) {
        // Tick the squad tracker (handles nag timer)
        self.squad_tracker.tick(&self.settings);

        // Render debug window in debug builds
        #[cfg(debug_assertions)]
        self.render_debug_window(ui);
    }

    #[cfg(debug_assertions)]
    fn render_debug_window(&mut self, ui: &Ui) {
        use arcdps::imgui::{Condition, Window};

        if !self.debug_window_visible {
            return;
        }

        Window::new("Squad Ready Debug")
            .opened(&mut self.debug_window_visible)
            .size([400.0, 300.0], Condition::FirstUseEver)
            .build(ui, || {
                ui.text(format!("Extras Loaded: {}", self.extras_loaded));
                ui.text(format!("Self Account: {}", self.self_account_name));
                ui.separator();

                let debug_info = self.squad_tracker.get_debug_info();
                ui.text(format!("In Ready Check: {}", debug_info.in_ready_check));
                ui.text(format!("Self Readied: {}", debug_info.self_readied));
                if let Some(elapsed) = debug_info.ready_check_elapsed_secs {
                    ui.text(format!("Ready Check Duration: {:.1}s", elapsed));
                }
                if let Some(until_nag) = debug_info.time_until_nag_secs {
                    ui.text(format!("Time Until Nag: {:.1}s", until_nag));
                }

                ui.separator();
                ui.text(format!(
                    "Cached Players: {}",
                    debug_info.cached_players.len()
                ));

                if !debug_info.cached_players.is_empty() {
                    ui.separator();
                    for (name, info) in &debug_info.cached_players {
                        let ready_marker = if info.ready_status { "✓" } else { "✗" };
                        ui.text(format!(
                            "{} {} [{}] (Sub {})",
                            ready_marker, name, info.role, info.subgroup
                        ));
                    }
                }
            });
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
