mod audio;
mod file_picker;
mod panic_handler;
mod plugin;
mod settings;
mod settings_ui;
mod squad_tracker;
mod update_checker;

use arcdps::extras::UserInfoIter;
use arcdps::imgui::Ui;
use log::*;
use plugin::Plugin;

use std::sync::{Mutex, MutexGuard};

use once_cell::sync::Lazy;

/// Extension trait to recover from poisoned mutexes instead of panicking.
pub trait MutexExt<T> {
    fn lock_or_recover(&self) -> MutexGuard<'_, T>;
}

impl<T> MutexExt<T> for Mutex<T> {
    fn lock_or_recover(&self) -> MutexGuard<'_, T> {
        self.lock().unwrap_or_else(|e| e.into_inner())
    }
}

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
    raw_wnd_nofilter,
}

/// Raw WndProc callback to capture the game's window handle.
/// This is called for every window message, we use it to get and store the HWND.
#[cfg(windows)]
unsafe extern "C-unwind" fn raw_wnd_nofilter(
    h_wnd: windows::Win32::Foundation::HWND,
    u_msg: u32,
    _w_param: windows::Win32::Foundation::WPARAM,
    _l_param: windows::Win32::Foundation::LPARAM,
) -> u32 {
    plugin::set_game_hwnd(h_wnd);
    u_msg
}

/// Stub for non-Windows platforms  
#[cfg(not(windows))]
unsafe extern "C-unwind" fn raw_wnd_nofilter(
    _h_wnd: usize,
    u_msg: u32,
    _w_param: usize,
    _l_param: isize,
) -> u32 {
    u_msg
}

fn imgui(ui: &Ui, not_loading_or_character_selection: bool) {
    PLUGIN
        .lock_or_recover()
        .render_windows(ui, not_loading_or_character_selection)
}

fn extras_init(addon_info: arcdps::extras::ExtrasAddonInfo, account_name: Option<&str>) {
    PLUGIN
        .lock_or_recover()
        .extras_init(&addon_info, account_name);
}

fn extras_squad_update(users: UserInfoIter) {
    PLUGIN.lock_or_recover().squad_update(users)
}

fn options_end(ui: &Ui) {
    PLUGIN.lock_or_recover().render_settings(ui)
}

fn init() -> Result<(), Option<String>> {
    debug!("arc init");
    panic_handler::install_panic_handler();

    PLUGIN
        .lock_or_recover()
        .load()
        .map_err(|e| Some(e.to_string()))
}

fn release() {
    debug!("release called");
    PLUGIN.lock_or_recover().release();
}
