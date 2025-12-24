#include "SquadTracker.h"

#include "Globals.h"
#include "Settings.h"

void SquadTracker::UpdateUsers(const UserInfo* updated_users,
                               const size_t updated_users_count) {
  std::scoped_lock guard(cached_players_mutex_);
  logging::Debug(
      std::format("received squad callback with {} users",
                  updated_users_count));
  for (size_t i = 0; i < updated_users_count; i++) {
    const auto user = updated_users[i];
    auto user_account_name = std::string(user.AccountName);
    if (user_account_name.at(0) == ':') {
      user_account_name.erase(0, 1);
    }
    logging::Debug(
        std::format("updated user {} accountname: {} ready: {} role: {} "
                    "jointime: {} subgroup: {}",
                    i, user.AccountName, user.ReadyStatus,
                    static_cast<uint8_t>(user.Role),
                    user.JoinTime, user.Subgroup));
    // User added/updated
    if (user.Role != UserRole::None) {
      if (user_account_name == globals::self_account_name) {
        self_readied_ = user.ReadyStatus;
      }
      if (auto old_user_it = cached_players_.find(user_account_name);
        old_user_it == cached_players_.end()) {
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
        self_readied_ = false;
        cached_players_.clear();
        ReadyCheckEnded();
      } else {
        // Remove player from cache
        cached_players_.erase(user_account_name);
      }
    }
  }
}

void SquadTracker::Tick() {
  // no active ready check
  if (!in_ready_check_) return;
  // self is already readied up
  if (self_readied_) return;
  Settings::f([this](const Settings& s) {
    // nag is disabled
    if (!s.settings.ready_check_nag) return;
    // nag time has not passed
    if (std::chrono::steady_clock::now() < ready_check_nag_time_) return;
    // nag time has passed, time to nag
    SetReadyCheckNagTime();
    FlashWindow();
    AudioPlayer::f([](AudioPlayer& i) { i.PlayReadyCheck(); });
  });
}

void SquadTracker::Draw() {
  if (!debug_window_visible_) {
    return;
  }

  ImGuiWindowFlags imGuiWindowFlags =
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing |
      ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoNavFocus |
      ImGuiWindowFlags_AlwaysAutoResize;

  ImGui::Begin("Squad Ready Debug", &debug_window_visible_, imGuiWindowFlags);
  ImGui::TextDisabled("Squad Members");

  ImGui::BeginTable("readydebug", 4);
  ImGui::TableSetupColumn("Account");
  ImGui::TableSetupColumn("Role");
  ImGui::TableSetupColumn("Sub");
  ImGui::TableSetupColumn("Ready");
  ImGui::TableHeadersRow();

  {
    std::scoped_lock guard(cached_players_mutex_);
    for (auto& [account_name, user_info] : cached_players_) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(account_name.c_str());
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(
          std::format("{}", static_cast<uint8_t>(user_info.Role)).c_str());
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(std::format("{}", user_info.Subgroup + 1).c_str());
      ImGui::TableNextColumn();
      if (user_info.ReadyStatus) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Ready");
      } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Not Ready");
      }
    }
  }

  ImGui::EndTable();

  ImGui::Separator();
  ImGui::TextDisabled("Internal Variables");

  if (in_ready_check_) {
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "in_ready_check_");
  } else {
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "in_ready_check_");
  }

  if (self_readied_) {
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "self_readied_");
  } else {
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "self_readied_");
  }

  ImGui::TextUnformatted(
      std::format("{} ready_check_start_time_",
                  ready_check_start_time_.time_since_epoch().count())
      .c_str());
  ImGui::TextUnformatted(
      std::format("{} ready_check_nag_time_",
                  ready_check_nag_time_.time_since_epoch().count())
      .c_str());
  ImGui::TextUnformatted(
      std::format("{} current_time",
                  std::chrono::steady_clock::now().time_since_epoch().count())
      .c_str());

  ImGui::Separator();
  ImGui::TextDisabled("Settings");

  Settings::f([this](Settings& s) {
    if (s.settings.ready_check_nag) {
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "ready_check_nag");
    } else {
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "ready_check_nag");
    }

    if (s.settings.ready_check_nag_in_combat) {
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                         "ready_check_nag_in_combat");
    } else {
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                         "ready_check_nag_in_combat");
    }

    ImGui::TextUnformatted(
        std::format(
            "{} ready_check_nag_interval_seconds",
            s.settings.ready_check_nag_interval_seconds)
            .c_str());
  });

  ImGui::End();
}

void SquadTracker::ReadyCheckStarted() {
  logging::Debug("ready check has started");
  in_ready_check_ = true;
  ready_check_start_time_ = std::chrono::steady_clock::now();
  SetReadyCheckNagTime();
  FlashWindow();
  AudioPlayer::f([](AudioPlayer& i) { i.PlayReadyCheck(); });
}

void SquadTracker::ReadyCheckCompleted() {
  logging::Debug("squad is ready");
  ready_check_nag_time_ = {};
  FlashWindow();
  AudioPlayer::f([](AudioPlayer& i) { i.PlaySquadReady(); });
  ReadyCheckEnded();
}

void SquadTracker::ReadyCheckEnded() {
  logging::Debug("ready check has ended");
  in_ready_check_ = false;
  ready_check_start_time_ = {};
}

void SquadTracker::SetReadyCheckNagTime() {
  Settings::f([this](Settings& s) {
    ready_check_nag_time_ =
        std::chrono::steady_clock::now() +
        std::chrono::milliseconds(static_cast<uint64_t>(
            1000 *
            s.settings.ready_check_nag_interval_seconds));
  });
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
  Settings::f([](Settings& s) {
    if (!s.settings.flash_window) return;
    const auto wnd = globals::some_window;
    if (wnd == nullptr) {
      return;
    }
    FLASHWINFO flash_info{sizeof(flash_info), wnd,
                          FLASHW_ALL | FLASHW_TIMERNOFG, 100, 0};
    FlashWindowEx(&flash_info);
  });
}
