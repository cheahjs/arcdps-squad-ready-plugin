#include "SquadTracker.h"

void SquadTracker::UpdateUsers(const UserInfo* updatedUsers,
                               size_t updatedUsersCount) {
  std::scoped_lock<std::mutex> guard(cached_players_mutex);
  logging::Debug(
      std::format("received squad callback with {} users", updatedUsersCount));
  for (size_t i = 0; i < updatedUsersCount; i++) {
    const auto user = updatedUsers[i];
    auto userAccountName = std::string(user.AccountName);
    if (userAccountName.at(0) == ':') {
      userAccountName.erase(0, 1);
    }
    logging::Debug(
        std::format("updated user {} accountname: {} ready: {} role: {} "
                    "jointime: {} subgroup: {}",
                    i, user.AccountName, user.ReadyStatus, (uint8_t)user.Role,
                    user.JoinTime, user.Subgroup));
    // User added/updated
    if (user.Role != UserRole::None) {
      auto oldUserIt = cached_players.find(userAccountName);
      if (oldUserIt == cached_players.end()) {
        // User added
        cached_players.emplace(userAccountName, user);
      } else {
        // User updated
        auto oldUser = oldUserIt->second;
        cached_players.insert_or_assign(userAccountName, user);

        if (user.Role == UserRole::SquadLeader) {
          if (user.ReadyStatus && !oldUser.ReadyStatus) {
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
      if (userAccountName == self_account_name) {
        // Self left squad, reset cache
        cached_players.clear();
      } else {
        // Remove player from cache
        cached_players.erase(userAccountName);
      }
    }
  }
}

void SquadTracker::ReadyCheckStarted() {
  logging::Debug("ready check has started");
  in_ready_check = true;
  audio_player->PlayReadyCheck();
}

void SquadTracker::ReadyCheckCompleted() {
  logging::Debug("squad is ready");
  audio_player->PlaySquadReady();
}

void SquadTracker::ReadyCheckEnded() {
  logging::Debug("ready check has ended");
  in_ready_check = false;
}

bool SquadTracker::AllPlayersReadied() {
  // iterate through all cachedPlayers, and check readyStatus == true and
  // in squad
  for (auto const& [accountName, user] : cached_players) {
    if (user.Role != UserRole::SquadLeader &&
        user.Role != UserRole::Lieutenant && user.Role != UserRole::Member) {
      logging::Debug(std::format("ignoring {} because they are role {}",
                                 accountName, (int)user.Role));
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
