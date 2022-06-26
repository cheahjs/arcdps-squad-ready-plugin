mod audio;
mod panic_handler;
mod squad_tracker;
mod ui;

use std::sync::Arc;
use std::sync::mpsc;
use std::sync::RwLock;
use std::thread::Builder;

use anyhow::Context;
use arcdps::arcdps_export;
use arcdps::imgui;
use arcdps::UserInfoIter;
use audio::{AudioAction, AudioPlayer};
use log::*;
use once_cell::sync::Lazy;
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
static mut AUDIO_CHANNEL: Option<mpsc::Sender<AudioAction>> = None;

static UI_STATE: Lazy<ui::SharedState> = Lazy::new(|| {
    Arc::new(RwLock::new(ui::State::new()))
});

fn unofficial_extras_init(
    self_account_name: Option<&str>,
    _unofficial_extras_version: Option<&'static str>,
) {
    if let Some(name) = self_account_name {
        let mut tracker = SQUAD_TRACKER.write();
        tracker.get_or_insert(SquadTracker::new(name, UI_STATE.clone()));

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

    debug!("Setting up audio");
    let (audio_send, audo_recv) = mpsc::channel();
    let mut sender = AUDIO_CHANNEL.write();
    sender.replace(audio_send);

    let audio_ui_state = UI_STATE.clone();
    let audio = Builder::new()
        .name("audio".to_owned())
        .spawn(move || match AudioPlayer::try_new(audo_recv, audio_ui_state) {
            Ok(mut audio_player) => {
                audio_player.start();
            }
            Err(error) => {
                error!("Failed to start audio player: {}", error.to_string());
            }
        })
        .context("Couldn't spawn audio thread");
    if audio.is_err() {
        error!("Failed to spawn audio thread");
        return Err(audio.err().unwrap().into());
    }

    Ok(())
}

fn release() {}

fn options_end(ui: &imgui::Ui) {}
