#pragma once

#include <map>
#include <mutex>

#include "Audio.h"
#include "Globals.h"
#include "Logging.h"
#include "../modules/unofficial_extras/Definitions.h"

class SquadTracker {
 private:
  AudioPlayer* audio_player;
  std::map<std::string, UserInfo> cached_players;
  std::mutex cached_players_mutex;
  bool in_ready_check;

 public:
  SquadTracker() : in_ready_check(false) {}
  void UpdateUsers(const UserInfo* updatedUsers, size_t updatedUsersCount);

 private:
  void ReadyCheckStarted();
  void ReadyCheckCompleted();
  void ReadyCheckEnded();
  bool AllPlayersReadied();
};
