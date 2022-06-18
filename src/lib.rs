mod audio;
mod panic_handler;
mod squad_tracker;

use arcdps::arcdps_export;
use arcdps::imgui;
use arcdps::UserInfoIter;
use audio::AudioPlayer;
use log::*;
use panic_handler::install_panic_handler;
use squad_tracker::SquadTracker;
use static_init::dynamic;

arcdps_export! {
    name: "Squad Ready",
    sig: 0xbcab9171u32,
    options_end: options_end,
    init: init,
    release: release,
    unofficial_extras_init: unofficial_extras_init,
    unofficial_extras_squad_update: unofficial_extras_squad_update,
}

#[dynamic]
static mut SQUAD_TRACKER: Option<SquadTracker> = None;

#[dynamic]
static mut AUDIO_PLAYER: Option<AudioPlayer> = None;

fn unofficial_extras_init(
    self_account_name: Option<&str>,
    _unofficial_extras_version: Option<&'static str>,
) {
    if let Some(name) = self_account_name {
        let mut tracker = SQUAD_TRACKER.write();
        tracker.get_or_insert(SquadTracker::new(name));

        debug!("Extras init");
    }
}

fn unofficial_extras_squad_update(users: UserInfoIter) {
    if let Some(tracker) = &mut *SQUAD_TRACKER.write() {
        tracker.squad_update(users, None, None);
    }
}

fn init() -> Result<(), Box<dyn std::error::Error>> {
    debug!("arc init");
    install_panic_handler();

    Ok(())
}

fn release() {}

fn options_end(ui: &imgui::Ui) {}
