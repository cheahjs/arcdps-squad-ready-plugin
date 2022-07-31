#include "SquadTracker.h"

#include "Globals.h"
#include "Settings.h"

void SquadTracker::UpdateUsers(const UserInfo* updated_users,
                               const size_t updated_users_count) {
  std::scoped_lock guard(cached_players_mutex_);
  logging::Debug(
      std::format("received squad callback with {} users", updated_users_count));
  for (size_t i = 0; i < updated_users_count; i++) {
    const auto user = updated_users[i];
    auto user_account_name = std::string(user.AccountName);
    if (user_account_name.at(0) == ':') {
      user_account_name.erase(0, 1);
    }
    logging::Debug(
        std::format("updated user {} accountname: {} ready: {} role: {} "
                    "jointime: {} subgroup: {}",
                    i, user.AccountName, user.ReadyStatus, static_cast<uint8_t>(user.Role),
                    user.JoinTime, user.Subgroup));
    // User added/updated
    if (user.Role != UserRole::None) {
      if (auto old_user_it = cached_players_.find(user_account_name); old_user_it == cached_players_.end()) {
        // User added
        cached_players_.emplace(user_account_name, user);
      } else {
        // User updated
        const auto old_user = old_user_it->second;
        cached_players_.insert_or_assign(user_account_name, user);

        if (user.Role == UserRole::SquadLeader) {
          if (user.ReadyStatus && !old_user.ReadyStatus) {
            // Squad leader has started a ready check
            ReadyCheckStarted();
          } else {
            // Squad leader has ended a ready check, either via a complete
            // ready check or by cancelling
            ReadyCheckEnded();
          }
        } else {
          if (AllPlayersReadied()) {
            ReadyCheckCompleted();
          }
        }
      }
    }
    // User removed
    else {
      if (user_account_name == globals::self_account_name) {
        // Self left squad, reset cache
        cached_players_.clear();
      } else {
        // Remove player from cache
        cached_players_.erase(user_account_name);
      }
    }
  }
}

void SquadTracker::ReadyCheckStarted() {
  logging::Debug("ready check has started");
  in_ready_check_ = true;
  ready_check_start_time_ =
      std::chrono::system_clock::now().time_since_epoch().count();
  FlashWindow();
  AudioPlayer::instance().PlayReadyCheck();
}

void SquadTracker::ReadyCheckCompleted() const {
  logging::Debug("squad is ready");
  FlashWindow();
  AudioPlayer::instance().PlaySquadReady();
}

void SquadTracker::ReadyCheckEnded() {
  logging::Debug("ready check has ended");
  in_ready_check_ = false;
  ready_check_start_time_ = 0;
}

bool SquadTracker::AllPlayersReadied() {
  // iterate through all cachedPlayers, and check readyStatus == true and
  // in squad
  for (auto const& [accountName, user] : cached_players_) {
    if (user.Role != UserRole::SquadLeader &&
        user.Role != UserRole::Lieutenant && user.Role != UserRole::Member) {
      logging::Debug(std::format("ignoring {} because they are role {}",
                                 accountName, static_cast<int>(user.Role)));
      continue;
    }
    if (!user.ReadyStatus) {
      logging::Debug(std::format("squad not ready due to {}", accountName));
      return false;
    }
  }
  logging::Debug("all players are readied");
  return true;
}

void SquadTracker::FlashWindow() {
  if (!Settings::instance().settings.flash_window) {
    return;
  }
  const auto wnd = globals::some_window;
  if (wnd == nullptr) {
    return;
  }
  FLASHWINFO flash_info{sizeof(flash_info), wnd,
                       FLASHW_ALL | FLASHW_TIMERNOFG, 100, 0};
  FlashWindowEx(&flash_info);
}
