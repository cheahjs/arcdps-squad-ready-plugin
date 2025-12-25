mod audio;
mod file_picker;
mod panic_handler;
mod plugin;
mod settings;
mod settings_ui;
mod squad_tracker;

use arcdps::extras::UserInfoIter;
use arcdps::imgui::Ui;
use log::*;
use plugin::Plugin;

use std::sync::Mutex;

use once_cell::sync::Lazy;

static PLUGIN: Lazy<Mutex<Plugin>> = Lazy::new(|| Mutex::new(Plugin::new()));

arcdps::export! {
    name: "Squad Ready",
    sig: 0xBCAB9171u32,
    init,
    release,
    options_end,
    imgui,
    extras_init,
    extras_squad_update,
}

fn imgui(ui: &Ui, not_loading_or_character_selection: bool) {
    PLUGIN
        .lock()
        .unwrap()
        .render_windows(ui, not_loading_or_character_selection)
}

fn extras_init(addon_info: arcdps::extras::ExtrasAddonInfo, account_name: Option<&str>) {
    PLUGIN
        .lock()
        .unwrap()
        .extras_init(&addon_info, account_name);
}

fn extras_squad_update(users: UserInfoIter) {
    PLUGIN.lock().unwrap().squad_update(users)
}

fn options_end(ui: &Ui) {
    PLUGIN.lock().unwrap().render_settings(ui)
}

fn init() -> Result<(), Option<String>> {
    debug!("arc init");
    panic_handler::install_panic_handler();

    PLUGIN
        .lock()
        .unwrap()
        .load()
        .map_err(|e| Some(e.to_string()))
}

fn release() {
    debug!("release called");
    PLUGIN.lock().unwrap().release();
}
