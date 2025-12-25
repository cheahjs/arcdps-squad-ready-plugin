use std::collections::HashMap;
use std::time::{Duration, Instant};

use arcdps::extras::UserRole;
use log::debug;

use crate::audio::sounds;
use crate::audio::AudioTrack;
use crate::plugin::AUDIO_PLAYER;
use crate::settings::Settings;

pub trait SquadUser {
    fn account_name(&self) -> Option<&str>;
    fn ready_status(&self) -> bool;
    fn role(&self) -> UserRole;
    fn subgroup(&self) -> u8;
}

impl SquadUser for arcdps::extras::UserInfo {
    fn account_name(&self) -> Option<&str> {
        arcdps::extras::UserInfo::account_name(self)
    }
    fn ready_status(&self) -> bool {
        self.ready_status
    }
    fn role(&self) -> UserRole {
        self.role
    }
    fn subgroup(&self) -> u8 {
        self.subgroup
    }
}

impl SquadUser for &arcdps::extras::UserInfo {
    fn account_name(&self) -> Option<&str> {
        arcdps::extras::UserInfo::account_name(self)
    }
    fn ready_status(&self) -> bool {
        self.ready_status
    }
    fn role(&self) -> UserRole {
        self.role
    }
    fn subgroup(&self) -> u8 {
        self.subgroup
    }
}

pub trait SquadNotifier {
    fn play_ready_check(&self, settings: &Settings);
    fn play_squad_ready(&self, settings: &Settings);
    fn flash_window(&self, settings: &Settings);
}

pub struct RealSquadNotifier;

impl SquadNotifier for RealSquadNotifier {
    fn flash_window(&self, settings: &Settings) {
        if !settings.flash_window {
            return;
        }

        // Flash the window using Windows API
        #[cfg(windows)]
        {
            use windows::Win32::UI::WindowsAndMessaging::{
                FlashWindowEx, GetForegroundWindow, FLASHWINFO, FLASHW_ALL, FLASHW_TIMERNOFG,
            };

            // Get the foreground window
            let hwnd = unsafe { GetForegroundWindow() };
            if !hwnd.is_invalid() {
                let mut flash_info = FLASHWINFO {
                    cbSize: std::mem::size_of::<FLASHWINFO>() as u32,
                    hwnd,
                    dwFlags: FLASHW_ALL | FLASHW_TIMERNOFG,
                    uCount: 100,
                    dwTimeout: 0,
                };

                unsafe {
                    let _ = FlashWindowEx(&mut flash_info);
                }
            }
        }
    }

    fn play_ready_check(&self, settings: &Settings) {
        let mut track = AudioTrack::new();
        let path = settings.ready_check_path.as_deref().unwrap_or("");
        let _ = track.load_from_path(path, sounds::READY_CHECK, settings.ready_check_volume);

        if track.is_valid() {
            AUDIO_PLAYER.lock().unwrap().play_track(&track);
        }
    }

    fn play_squad_ready(&self, settings: &Settings) {
        let mut track = AudioTrack::new();
        let path = settings.squad_ready_path.as_deref().unwrap_or("");
        let _ = track.load_from_path(path, sounds::SQUAD_READY, settings.squad_ready_volume);

        if track.is_valid() {
            AUDIO_PLAYER.lock().unwrap().play_track(&track);
        }
    }
}

pub struct SquadTracker<N: SquadNotifier = RealSquadNotifier> {
    cached_players: HashMap<String, CachedUserInfo>,
    ready_check_start_time: Option<Instant>,
    ready_check_nag_time: Option<Instant>,
    in_ready_check: bool,
    self_readied: bool,
    notifier: N,
}

#[derive(Clone, Debug)]
#[allow(dead_code)]
struct CachedUserInfo {
    role: UserRole,
    subgroup: u8,
    ready_status: bool,
}

impl SquadTracker<RealSquadNotifier> {
    pub fn new() -> Self {
        Self::with_notifier(RealSquadNotifier)
    }
}

impl<N: SquadNotifier> SquadTracker<N> {
    pub fn with_notifier(notifier: N) -> Self {
        Self {
            cached_players: HashMap::new(),
            ready_check_start_time: None,
            ready_check_nag_time: None,
            in_ready_check: false,
            self_readied: false,
            notifier,
        }
    }

    pub fn update_users<U: SquadUser, I: IntoIterator<Item = U>>(
        &mut self,
        users: I,
        self_account_name: &str,
        settings: &Settings,
    ) {
        debug!("received squad update");

        for user in users {
            let account_name = match user.account_name() {
                Some(name) => arcdps::strip_account_prefix(name).to_string(),
                None => continue,
            };

            debug!(
                "updated user: {} ready: {} role: {:?} subgroup: {}",
                account_name,
                user.ready_status(),
                user.role(),
                user.subgroup()
            );

            // User added/updated
            if user.role() != UserRole::None {
                if account_name == self_account_name {
                    self.self_readied = user.ready_status();
                }

                let old_user = self.cached_players.get(&account_name).cloned();

                // Update cache
                self.cached_players.insert(
                    account_name.clone(),
                    CachedUserInfo {
                        role: user.role(),
                        subgroup: user.subgroup(),
                        ready_status: user.ready_status(),
                    },
                );

                if let Some(old_user) = old_user {
                    // User updated
                    if user.role() == UserRole::SquadLeader {
                        if user.ready_status() && !old_user.ready_status {
                            // Squad leader has started a ready check
                            self.ready_check_started(settings);
                        } else if !user.ready_status() && old_user.ready_status {
                            // Squad leader has ended a ready check
                            self.ready_check_ended();
                        }
                    } else if self.in_ready_check && self.all_players_readied() {
                        self.ready_check_completed(settings);
                    }
                }
            } else {
                // User removed
                if account_name == self_account_name {
                    // Self left squad, reset cache
                    self.self_readied = false;
                    self.cached_players.clear();
                    self.ready_check_ended();
                } else {
                    // Remove player from cache
                    self.cached_players.remove(&account_name);
                }
            }
        }
    }

    pub fn tick(&mut self, settings: &Settings) {
        // no active ready check
        if !self.in_ready_check {
            return;
        }

        // self is already readied up
        if self.self_readied {
            return;
        }

        // nag is disabled
        if !settings.ready_check_nag {
            return;
        }

        // nag time has not passed
        if let Some(nag_time) = self.ready_check_nag_time {
            if Instant::now() < nag_time {
                return;
            }
        }

        // nag time has passed, time to nag
        self.set_ready_check_nag_time(settings);
        self.notifier.flash_window(settings);
        self.notifier.play_ready_check(settings);
    }

    fn ready_check_started(&mut self, settings: &Settings) {
        debug!("ready check has started");
        self.in_ready_check = true;
        self.ready_check_start_time = Some(Instant::now());
        self.set_ready_check_nag_time(settings);
        self.notifier.flash_window(settings);
        self.notifier.play_ready_check(settings);
    }

    fn ready_check_completed(&mut self, settings: &Settings) {
        debug!("squad is ready");
        self.ready_check_nag_time = None;
        self.notifier.flash_window(settings);
        self.notifier.play_squad_ready(settings);
        self.ready_check_ended();
    }

    fn ready_check_ended(&mut self) {
        debug!("ready check has ended");
        self.in_ready_check = false;
        self.ready_check_start_time = None;
    }

    fn set_ready_check_nag_time(&mut self, settings: &Settings) {
        // Clamp to >= 0.0 to prevent panic from Duration::from_secs_f32 on negative values
        let interval = settings.ready_check_nag_interval_seconds.max(0.0);
        let nag_duration = Duration::from_secs_f32(interval);
        self.ready_check_nag_time = Some(Instant::now() + nag_duration);
    }

    fn all_players_readied(&self) -> bool {
        for (account_name, user) in &self.cached_players {
            // Only check squad members (not invited/applied)
            if user.role != UserRole::SquadLeader
                && user.role != UserRole::Lieutenant
                && user.role != UserRole::Member
            {
                debug!(
                    "ignoring {} because they are role {:?}",
                    account_name, user.role
                );
                continue;
            }

            if !user.ready_status {
                debug!("squad not ready due to {}", account_name);
                return false;
            }
        }
        debug!("all players are readied");
        true
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::sync::Arc;

    struct MockUser {
        account_name: Option<String>,
        ready_status: bool,
        role: UserRole,
        subgroup: u8,
    }

    impl SquadUser for MockUser {
        fn account_name(&self) -> Option<&str> {
            self.account_name.as_deref()
        }
        fn ready_status(&self) -> bool {
            self.ready_status
        }
        fn role(&self) -> UserRole {
            self.role
        }
        fn subgroup(&self) -> u8 {
            self.subgroup
        }
    }

    #[derive(Default, Clone)]
    struct MockNotifier {
        ready_check_plays: Arc<AtomicUsize>,
        squad_ready_plays: Arc<AtomicUsize>,
        flash_calls: Arc<AtomicUsize>,
    }

    impl SquadNotifier for MockNotifier {
        fn play_ready_check(&self, _settings: &Settings) {
            self.ready_check_plays.fetch_add(1, Ordering::SeqCst);
        }
        fn play_squad_ready(&self, _settings: &Settings) {
            self.squad_ready_plays.fetch_add(1, Ordering::SeqCst);
        }
        fn flash_window(&self, _settings: &Settings) {
            self.flash_calls.fetch_add(1, Ordering::SeqCst);
        }
    }

    #[test]
    fn test_ready_check_starts_when_leader_readies() {
        let notifier = MockNotifier::default();
        let mut tracker = SquadTracker::with_notifier(notifier.clone());
        let settings = Settings::default();

        let users = vec![MockUser {
            account_name: Some(":Leader.1234".to_string()),
            ready_status: false,
            role: UserRole::SquadLeader,
            subgroup: 1,
        }];

        // Initial state
        tracker.update_users(users, "Self.5678", &settings);
        assert_eq!(notifier.ready_check_plays.load(Ordering::SeqCst), 0);

        // Leader readies up
        let users = vec![MockUser {
            account_name: Some(":Leader.1234".to_string()),
            ready_status: true,
            role: UserRole::SquadLeader,
            subgroup: 1,
        }];
        tracker.update_users(users, "Self.5678", &settings);

        assert_eq!(notifier.ready_check_plays.load(Ordering::SeqCst), 1);
        assert!(tracker.in_ready_check);
    }

    #[test]
    fn test_ready_check_completes_when_everyone_readies() {
        let notifier = MockNotifier::default();
        let mut tracker = SquadTracker::with_notifier(notifier.clone());
        let settings = Settings::default();

        // 1. Leader starts ready check
        let users = vec![
            MockUser {
                account_name: Some(":Leader.1234".to_string()),
                ready_status: false,
                role: UserRole::SquadLeader,
                subgroup: 1,
            },
            MockUser {
                account_name: Some(":Member.1".to_string()),
                ready_status: false,
                role: UserRole::Member,
                subgroup: 1,
            },
        ];
        tracker.update_users(users, "Leader.1234", &settings);

        let users = vec![MockUser {
            account_name: Some(":Leader.1234".to_string()),
            ready_status: true,
            role: UserRole::SquadLeader,
            subgroup: 1,
        }];
        tracker.update_users(users, "Leader.1234", &settings);
        assert!(tracker.in_ready_check);

        // 2. Member readies up
        let users = vec![MockUser {
            account_name: Some(":Member.1".to_string()),
            ready_status: true,
            role: UserRole::Member,
            subgroup: 1,
        }];
        tracker.update_users(users, "Leader.1234", &settings);

        assert_eq!(notifier.squad_ready_plays.load(Ordering::SeqCst), 1);
        assert!(!tracker.in_ready_check);
    }

    #[test]
    fn test_nag_logic() {
        let notifier = MockNotifier::default();
        let mut tracker = SquadTracker::with_notifier(notifier.clone());
        let mut settings = Settings::default();
        settings.ready_check_nag = true;
        settings.ready_check_nag_interval_seconds = 0.0; // Instant nag for testing

        // Start ready check
        let users = vec![MockUser {
            account_name: Some(":Leader.1234".to_string()),
            ready_status: true,
            role: UserRole::SquadLeader,
            subgroup: 1,
        }];
        tracker.update_users(users, "Self.5678", &settings);

        // Initial start calls: 1 flash, 1 ready check play
        let initial_plays = notifier.ready_check_plays.load(Ordering::SeqCst);
        assert!(initial_plays >= 1);

        // Tick should trigger nag
        tracker.tick(&settings);
        assert_eq!(
            notifier.ready_check_plays.load(Ordering::SeqCst),
            initial_plays + 1
        );
    }

    #[test]
    fn test_self_leaving_squad_resets_tracker() {
        let notifier = MockNotifier::default();
        let mut tracker = SquadTracker::with_notifier(notifier.clone());
        let settings = Settings::default();

        tracker.update_users(
            vec![MockUser {
                account_name: Some(":Leader.1234".to_string()),
                ready_status: true,
                role: UserRole::SquadLeader,
                subgroup: 1,
            }],
            "Self.5678",
            &settings,
        );
        assert!(tracker.in_ready_check);

        // Self leaves (role becomes None or removed)
        tracker.update_users(
            vec![MockUser {
                account_name: Some(":Self.5678".to_string()),
                ready_status: false,
                role: UserRole::None,
                subgroup: 0,
            }],
            "Self.5678",
            &settings,
        );

        assert!(!tracker.in_ready_check);
        assert!(tracker.cached_players.is_empty());
    }

    #[test]
    fn test_squad_ready_not_fired_without_active_ready_check() {
        let notifier = MockNotifier::default();
        let mut tracker = SquadTracker::with_notifier(notifier.clone());
        let settings = Settings::default();

        // 1. Initialize squad with leader (not readied) and a member (not readied)
        let users = vec![
            MockUser {
                account_name: Some(":Leader.1234".to_string()),
                ready_status: false,
                role: UserRole::SquadLeader,
                subgroup: 1,
            },
            MockUser {
                account_name: Some(":Member.1".to_string()),
                ready_status: false,
                role: UserRole::Member,
                subgroup: 1,
            },
        ];
        tracker.update_users(users, "Self.5678", &settings);
        assert!(!tracker.in_ready_check);

        // 2. Member readies up without an active ready check which should only happen if we desynced
        let users = vec![MockUser {
            account_name: Some(":Member.1".to_string()),
            ready_status: true,
            role: UserRole::Member,
            subgroup: 1,
        }];
        tracker.update_users(users, "Self.5678", &settings);

        // Squad ready should NOT have been played since no ready check is active
        assert_eq!(notifier.squad_ready_plays.load(Ordering::SeqCst), 0);
        assert!(!tracker.in_ready_check);
    }
}
