#pragma once

#include <map>
#include <mutex>

#include "Audio.h"
#include "unofficial_extras/Definitions.h"

class SquadTracker {
  std::map<std::string, UserInfo> cached_players_;
  std::mutex cached_players_mutex_;
  std::chrono::time_point<std::chrono::steady_clock> ready_check_start_time_;
  std::chrono::time_point<std::chrono::steady_clock> ready_check_nag_time_;
  bool in_ready_check_;
  bool self_readied_;

 public:
  SquadTracker() : in_ready_check_(false), self_readied_(false) {}
  void UpdateUsers(const UserInfo* updated_users, size_t updated_users_count);
  void Tick();

 private:
  void ReadyCheckStarted();
  void ReadyCheckCompleted();
  void ReadyCheckEnded();
  bool AllPlayersReadied();
  static void FlashWindow();
};
