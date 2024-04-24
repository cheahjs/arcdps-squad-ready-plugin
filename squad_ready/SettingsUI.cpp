#include "SettingsUI.h"

#include <string>

#include "Audio.h"
#include "Globals.h"
#include "Settings.h"
#include "extension/imgui_stdlib.h"
#include "imgui/imgui.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"

void DrawReadyCheck() {
  Settings::instance([](Settings& settings) {
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Ready Check");

    // Volume
    int& ready_check_volume = settings.settings.ready_check_volume;
    if (ImGui::SliderInt("Volume - Ready Check", &ready_check_volume, 0, 100,
                         "%d%%")) {
      AudioPlayer::instance([&](AudioPlayer& audio_player) {
        audio_player.UpdateReadyCheckVolume(ready_check_volume);
      });
    }

    // Path
    std::string ready_check_path =
        settings.settings.ready_check_path.value_or("");
    if (ImGui::InputText(
            "Path to file to play on ready check (blank for default)",
            &ready_check_path)) {
      if (ready_check_path.empty()) {
        settings.settings.ready_check_path.reset();
      } else {
        settings.settings.ready_check_path = ready_check_path;
      }
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        AudioPlayer::instance([&](AudioPlayer& audio_player) {
          audio_player.UpdateReadyCheck(
              settings.settings.ready_check_path.value_or(""));
        });
      }
    }

    // Path - Dialog
    if (ImGui::Button("Open Ready Check File")) {
      ImGuiFileDialog::Instance()->OpenDialog(
          "ChooseReadyCheckFileDlgKey", "Choose Ready Check File", ".*",
          settings.settings.ready_check_path.value_or("."), 1, nullptr,
          ImGuiFileDialogFlags_Modal);
    }
    if (ImGuiFileDialog::Instance()->Display("ChooseReadyCheckFileDlgKey", 0,
                                             ImVec2(400, 200))) {
      if (ImGuiFileDialog::Instance()->IsOk()) {
        settings.settings.ready_check_path =
            ImGuiFileDialog::Instance()->GetFilePathName();
      }
      AudioPlayer::instance([&](AudioPlayer& audio_player) {
        audio_player.UpdateReadyCheck(
            settings.settings.ready_check_path.value_or(""));
      });
      ImGuiFileDialog::Instance()->Close();
    }

    // Play button for testing
    ImGui::SameLine();
    if (ImGui::Button("Play Ready Check")) {
      AudioPlayer::instance([&](AudioPlayer& audio_player) {
        if (audio_player.UpdateReadyCheck(
                settings.settings.ready_check_path.value_or(""))) {
          audio_player.PlayReadyCheck();
        }
      });
    }

    // Status of file
    AudioPlayer::instance([&](AudioPlayer& audio_player) {
      const auto ready_check_status =
          audio_player.ReadyCheckStatus();
      if (!ready_check_status.empty()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                           ready_check_status.c_str());
      }
    });

    // Nag options
    bool& nag = settings.settings.ready_check_nag;
    // bool& nag_in_combat = settings.settings.ready_check_nag_in_combat;
    float& nag_interval = settings.settings.ready_check_nag_interval_seconds;
    ImGui::Checkbox("Nag if not readied", &nag);
    // ImGui::Checkbox("Nag in combat", &nag_in_combat);
    ImGui::InputFloat("Nag interval in seconds", &nag_interval, 0.1f, 0,
                      "%.1f");
  });
}

void DrawSquadReady() {
  Settings::instance([](Settings& settings) {
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Squad Ready");

    int& squad_ready_volume = settings.settings.squad_ready_volume;
    if (ImGui::SliderInt("Volume - Squad Ready", &squad_ready_volume, 0, 100,
                         "%d%%")) {
      AudioPlayer::instance([&](AudioPlayer& audio_player) {
        audio_player.UpdateSquadReadyVolume(squad_ready_volume);
      });
    }

    std::string squad_ready_path =
        settings.settings.squad_ready_path.value_or("");
    if (ImGui::InputText(
            "Path to file to play on squad ready (blank for default)",
            &squad_ready_path)) {
      if (squad_ready_path.empty()) {
        settings.settings.squad_ready_path.reset();
      } else {
        settings.settings.squad_ready_path = squad_ready_path;
      }
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        AudioPlayer::instance([&](AudioPlayer& audio_player) {
          audio_player.UpdateSquadReady(
              settings.settings.squad_ready_path.value_or(""));
        });
      }
    }

    if (ImGui::Button("Open Squad Ready File")) {
      ImGuiFileDialog::Instance()->OpenDialog(
          "ChooseSquadReadyFileDlgKey", "Choose Squad Ready File", ".*",
          settings.settings.squad_ready_path.value_or("."), 1, nullptr,
          ImGuiFileDialogFlags_Modal);
    }
    if (ImGuiFileDialog::Instance()->Display("ChooseSquadReadyFileDlgKey", 0,
                                             ImVec2(400, 200))) {
      if (ImGuiFileDialog::Instance()->IsOk()) {
        settings.settings.squad_ready_path =
            ImGuiFileDialog::Instance()->GetFilePathName();
      }
      AudioPlayer::instance([&](AudioPlayer& audio_player) {
        audio_player.UpdateSquadReady(
            settings.settings.squad_ready_path.value_or(""));
      });
      ImGuiFileDialog::Instance()->Close();
    }
    ImGui::SameLine();
    if (ImGui::Button("Play Squad Ready")) {
      AudioPlayer::instance([&](AudioPlayer& audio_player) {
        if (audio_player.UpdateSquadReady(
                settings.settings.squad_ready_path.value_or(""))) {
          audio_player.PlaySquadReady();
        }
      });
    }
    AudioPlayer::instance([&](AudioPlayer& audio_player) {
      const auto squad_ready_status = audio_player.SquadReadyStatus();
      if (!squad_ready_status.empty()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                           squad_ready_status.c_str());
      }
    });
  });
}

void DrawGlobalSettings() {
  Settings::instance([](Settings& settings) {
    bool& flash_window = settings.settings.flash_window;
    ImGui::Checkbox("Flash window and tray icon", &flash_window);
  });
}

void DrawStatus(std::unique_ptr<SquadTracker>& tracker) {
  ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Status");

  ImGui::BeginGroup();
  ImGui::Text("Unofficial extras status: ");
  ImGui::SameLine();
  if (globals::unofficial_extras_loaded) {
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Loaded");
  } else {
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Not Loaded");
  }
  ImGui::EndGroup();
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(
        "Unofficial extras is required for receiving squad member updates.");
  }

  AudioPlayer::instance([](AudioPlayer& audio_player) {
    Settings::instance([&](Settings& settings) {
      std::vector<std::string> devices = audio_player.OutputDevices();
      const auto preview_value =
          settings.settings.audio_output_device.value_or("Default");

      if (ImGui::BeginCombo("Output device", preview_value.c_str())) {
        for (const auto& device : devices) {
          const bool is_selected = (preview_value == device);
          if (ImGui::Selectable(device.c_str(), is_selected)) {
            if (preview_value != device) {
              settings.settings.audio_output_device = device;
              audio_player.UpdateOutputDevice(device);
              audio_player.ReInit();
            }
          }
          if (is_selected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      if (ImGui::Button("Refresh Audio Devices")) {
        audio_player.UpdateOutputDevices();
      }
    });

    ImGui::Text(std::format("Current output device: {}", audio_player.OutputDeviceName())
                    .c_str());
    if (ImGui::Button("Reset Audio")) {
      audio_player.ReInit();
    }
  });
  if (ImGui::Button("Open Debug Window")) {
    tracker->MakeDebugWindowVisible();
  }
}

void SettingsUI::Draw(std::unique_ptr<SquadTracker>& tracker) {
  ImGui::Separator();
  ImGui::Spacing();
  DrawReadyCheck();

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
  DrawSquadReady();

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
  DrawGlobalSettings();

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
  DrawStatus(tracker);
}
