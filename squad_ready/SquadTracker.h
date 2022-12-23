#pragma once

#include <map>
#include <mutex>

#include "Audio.h"
#include "unofficial_extras/Definitions.h"

class SquadTracker {
  std::map<std::string, UserInfo> cached_players_;
  std::mutex cached_players_mutex_;
  uint64_t ready_check_start_time_;
  bool in_ready_check_;

 public:
  SquadTracker() : in_ready_check_(false) {}
  void UpdateUsers(const UserInfo* updated_users, size_t updated_users_count);

 private:
  void ReadyCheckStarted();
  void ReadyCheckCompleted() const;
  void ReadyCheckEnded();
  bool AllPlayersReadied();
  static void FlashWindow();
};
