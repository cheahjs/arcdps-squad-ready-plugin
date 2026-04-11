use arcdps::imgui::{InputFloat, Selectable, Slider, Ui};
use log::debug;
use once_cell::sync::Lazy;
use rodio::cpal::traits::{DeviceTrait, HostTrait};
use std::path::PathBuf;
use std::sync::Mutex;
use std::time::Instant;

use crate::audio::sounds;
use crate::audio::AudioTrack;
use crate::file_picker::FilePicker;
use crate::plugin::AUDIO_PLAYER;
use crate::settings::{Settings, SoundStatus};

/// Cached output device list to avoid querying the audio API every frame.
struct DeviceCache {
    devices: Vec<String>,
    last_refresh: Instant,
}

impl DeviceCache {
    fn new() -> Self {
        Self {
            devices: query_output_devices(),
            last_refresh: Instant::now(),
        }
    }

    fn get(&mut self) -> Vec<String> {
        if self.last_refresh.elapsed().as_secs() > 5 {
            self.devices = query_output_devices();
            self.last_refresh = Instant::now();
        }
        self.devices.clone()
    }

    fn invalidate(&mut self) {
        self.devices = query_output_devices();
        self.last_refresh = Instant::now();
    }
}

static DEVICE_CACHE: Lazy<Mutex<DeviceCache>> = Lazy::new(|| Mutex::new(DeviceCache::new()));

fn get_output_devices() -> Vec<String> {
    DEVICE_CACHE.lock().unwrap().get()
}

fn invalidate_device_cache() {
    DEVICE_CACHE.lock().unwrap().invalidate();
}

pub fn draw(
    ui: &Ui,
    settings: &mut Settings,
    ready_check_picker: &mut FilePicker,
    squad_ready_picker: &mut FilePicker,
    extras_loaded: bool,
    #[cfg(debug_assertions)] debug_window_visible: &mut bool,
) {
    ui.separator();
    ui.spacing();

    draw_ready_check(ui, settings, ready_check_picker);

    ui.spacing();
    ui.separator();
    ui.spacing();

    draw_squad_ready(ui, settings, squad_ready_picker);

    ui.spacing();
    ui.separator();
    ui.spacing();

    draw_global_settings(ui, settings);

    ui.spacing();
    ui.separator();
    ui.spacing();

    #[cfg(debug_assertions)]
    draw_status(ui, settings, extras_loaded, debug_window_visible);
    #[cfg(not(debug_assertions))]
    draw_status(ui, settings, extras_loaded);

    ui.spacing();
    ui.separator();
    ui.spacing();

    draw_update_settings(ui, settings);
}

fn draw_ready_check(ui: &Ui, settings: &mut Settings, picker: &mut FilePicker) {
    ui.text_disabled("Ready Check");

    // Volume
    Slider::new("Volume - Ready Check", 0, 100).build(ui, &mut settings.ready_check_volume);

    // Path input (inline label)
    let mut path = settings.ready_check_path.clone().unwrap_or_default();
    if ui
        .input_text(
            "Path to file to play on ready check (blank for default)",
            &mut path,
        )
        .build()
    {
        if path.is_empty() {
            settings.ready_check_path = None;
        } else {
            settings.ready_check_path = Some(path);
        }
    }

    // Open file button
    if ui.button("Open Ready Check File") {
        if let Some(ref path) = settings.ready_check_path {
            let p = PathBuf::from(path);
            if let Some(parent) = p.parent() {
                if parent.is_dir() {
                    picker.current_dir = parent.to_path_buf();
                }
            }
        }
        picker.open();
    }

    if let Some(selected) = picker.render(ui) {
        settings.ready_check_path = Some(selected.to_string_lossy().to_string());
    }

    // Play button for testing (same line as Open button)
    ui.same_line();
    if ui.button("Play Ready Check") {
        let mut track = AudioTrack::new();
        let path = settings.ready_check_path.as_deref().unwrap_or("");
        if track
            .load_from_path(path, sounds::READY_CHECK, settings.ready_check_volume)
            .is_ok()
            && track.is_valid()
        {
            AUDIO_PLAYER.lock().unwrap().play_track(&track);
        }
    }

    // Show status (same line as Play button) - use cached status to avoid per-frame disk I/O
    let status = get_or_refresh_status(
        &mut settings.ready_check_status,
        &settings.ready_check_path,
        settings.ready_check_volume,
        sounds::READY_CHECK,
    );
    if !status.valid && !status.message.is_empty() {
        ui.same_line();
        ui.text_colored([1.0, 0.0, 0.0, 1.0], &status.message);
    }

    // Nag options
    ui.checkbox("Nag if not readied", &mut settings.ready_check_nag);
    InputFloat::new(
        ui,
        "Nag interval in seconds",
        &mut settings.ready_check_nag_interval_seconds,
    )
    .step(0.1)
    .build();
}

fn draw_squad_ready(ui: &Ui, settings: &mut Settings, picker: &mut FilePicker) {
    ui.text_disabled("Squad Ready");

    // Volume
    Slider::new("Volume - Squad Ready", 0, 100).build(ui, &mut settings.squad_ready_volume);

    // Path input (inline label)
    let mut path = settings.squad_ready_path.clone().unwrap_or_default();
    if ui
        .input_text(
            "Path to file to play on squad ready (blank for default)",
            &mut path,
        )
        .build()
    {
        if path.is_empty() {
            settings.squad_ready_path = None;
        } else {
            settings.squad_ready_path = Some(path);
        }
    }

    // Open file button
    if ui.button("Open Squad Ready File") {
        if let Some(ref path) = settings.squad_ready_path {
            let p = PathBuf::from(path);
            if let Some(parent) = p.parent() {
                if parent.is_dir() {
                    picker.current_dir = parent.to_path_buf();
                }
            }
        }
        picker.open();
    }

    if let Some(selected) = picker.render(ui) {
        settings.squad_ready_path = Some(selected.to_string_lossy().to_string());
    }

    // Play button for testing (same line as Open button)
    ui.same_line();
    if ui.button("Play Squad Ready") {
        let mut track = AudioTrack::new();
        let path = settings.squad_ready_path.as_deref().unwrap_or("");
        if track
            .load_from_path(path, sounds::SQUAD_READY, settings.squad_ready_volume)
            .is_ok()
            && track.is_valid()
        {
            AUDIO_PLAYER.lock().unwrap().play_track(&track);
        }
    }

    // Show status (same line as Play button) - use cached status to avoid per-frame disk I/O
    let status = get_or_refresh_status(
        &mut settings.squad_ready_status,
        &settings.squad_ready_path,
        settings.squad_ready_volume,
        sounds::SQUAD_READY,
    );
    if !status.valid && !status.message.is_empty() {
        ui.same_line();
        ui.text_colored([1.0, 0.0, 0.0, 1.0], &status.message);
    }
}

fn draw_global_settings(ui: &Ui, settings: &mut Settings) {
    ui.checkbox("Flash window and tray icon", &mut settings.flash_window);
}

fn draw_update_settings(ui: &Ui, settings: &mut Settings) {
    ui.text_disabled("Updates");

    ui.checkbox("Check for updates", &mut settings.check_for_updates);

    ui.checkbox(
        "Include pre-release versions",
        &mut settings.include_prereleases,
    );
    if ui.is_item_hovered() {
        ui.tooltip_text(
            "When enabled, includes beta/alpha releases when checking for updates. Requires restart.",
        );
    }
}

#[cfg(debug_assertions)]
fn draw_status(
    ui: &Ui,
    settings: &mut Settings,
    extras_loaded: bool,
    debug_window_visible: &mut bool,
) {
    draw_status_common(ui, settings, extras_loaded);

    // Debug window toggle (only in debug builds)
    ui.checkbox("Show Debug Window", debug_window_visible);
}

#[cfg(not(debug_assertions))]
fn draw_status(ui: &Ui, settings: &mut Settings, extras_loaded: bool) {
    draw_status_common(ui, settings, extras_loaded);
}

fn draw_status_common(ui: &Ui, settings: &mut Settings, extras_loaded: bool) {
    ui.text_disabled("Status");

    // Unofficial extras status
    ui.text("Unofficial extras status: ");
    ui.same_line();
    if extras_loaded {
        ui.text_colored([0.0, 1.0, 0.0, 1.0], "Loaded");
    } else {
        ui.text_colored([1.0, 0.0, 0.0, 1.0], "Not Loaded");
    }
    if ui.is_item_hovered() {
        ui.tooltip_text("Unofficial extras is required for receiving squad member updates.");
    }

    // Output device selection (cached, refreshes every 5 seconds)
    let devices = get_output_devices();
    let current_device = settings
        .audio_output_device
        .clone()
        .unwrap_or_else(|| "Default".to_string());

    let preview = current_device.as_str();
    if let Some(_combo) = ui.begin_combo("Output device", preview) {
        // Default option
        let is_default_selected = settings.audio_output_device.is_none();
        if Selectable::new("Default")
            .selected(is_default_selected)
            .build(ui)
            && settings.audio_output_device.is_some()
        {
            settings.audio_output_device = None;
            AUDIO_PLAYER.lock().unwrap().set_device(None);
        }

        // Device options
        for device in &devices {
            let selected = settings.audio_output_device.as_ref() == Some(device);
            if Selectable::new(device).selected(selected).build(ui)
                && settings.audio_output_device.as_ref() != Some(device)
            {
                settings.audio_output_device = Some(device.clone());
                AUDIO_PLAYER
                    .lock()
                    .unwrap()
                    .set_device(Some(device.clone()));
            }
        }
    }

    // Reset audio button
    if ui.button("Reset Audio") {
        invalidate_device_cache();
        let device = settings.audio_output_device.clone();
        AUDIO_PLAYER.lock().unwrap().set_device(device);
    }
}

/// Returns cached sound status, refreshing only when the path or volume changes.
fn get_or_refresh_status(
    cached: &mut Option<SoundStatus>,
    path: &Option<String>,
    volume: i32,
    default: &'static [u8],
) -> SoundStatus {
    if let Some(ref status) = cached {
        if status.path.as_ref() == path.as_ref() && status.volume == volume {
            return status.clone();
        }
    }

    let mut track = AudioTrack::new();
    let path_str = path.as_deref().unwrap_or("");
    let _ = track.load_from_path(path_str, default, volume);
    let status = SoundStatus {
        path: path.clone(),
        volume,
        valid: track.is_valid(),
        message: track.status_message,
    };
    *cached = Some(status.clone());
    status
}

fn query_output_devices() -> Vec<String> {
    let host = rodio::cpal::default_host();
    match host.output_devices() {
        Ok(devices) => devices.filter_map(|d| d.name().ok()).collect(),
        Err(e) => {
            debug!("failed to list output devices: {:#}", e);
            Vec::new()
        }
    }
}
