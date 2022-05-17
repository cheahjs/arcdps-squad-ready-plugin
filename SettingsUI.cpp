#include "SettingsUI.h"

#include <Windows.h>

#include "Audio.h"
#include "Globals.h"
#include "Settings.h"
#include "extension/KeyBindHandler.h"
#include "extension/KeyInput.h"
#include "extension/Widgets.h"
#include "extension/imgui_stdlib.h"
#include "imgui/imgui.h"

SettingsUI settingsUI;

void SettingsUI::Draw() {
  Settings& settings = Settings::instance();

  int& ready_check_volume = settings.settings.ready_check_volume;
  if (ImGui::SliderInt("Volume - Ready Check", &ready_check_volume, 0, 100,
                       "%d%%")) {
    // TODO: Update AudioPlayer volume
  }

  std::string ready_check_path =
      settings.settings.ready_check_path.value_or("");
  if (ImGui::InputText("Path to ready check .wav file (blank for default)",
                       &ready_check_path)) {
    if (ready_check_path.empty()) {
      settings.settings.ready_check_path.reset();
    } else {
      settings.settings.ready_check_path = ready_check_path;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      if (!AudioPlayer::instance().UpdateReadyCheck(
              settings.settings.ready_check_path.value_or(""))) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                           "Failed to load file");
      }
    }
  }

  if (ImGui::Button("Play Ready Check")) {
    if (!AudioPlayer::instance().UpdateReadyCheck(
            settings.settings.ready_check_path.value_or(""))) {
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to load file");
    } else {
      AudioPlayer::instance().PlayReadyCheck();
    }
  }

  int& squad_ready_volume = settings.settings.squad_ready_volume;
  if (ImGui::SliderInt("Volume - Squad Ready", &squad_ready_volume, 0, 100,
                       "%d%%")) {
    // TODO: Update AudioPlayer volume
  }

  std::string squad_ready_path =
      settings.settings.squad_ready_path.value_or("");
  if (ImGui::InputText("Path to squad ready .wav file (blank for default)",
                       &squad_ready_path)) {
    if (squad_ready_path.empty()) {
      settings.settings.squad_ready_path.reset();
    } else {
      settings.settings.squad_ready_path = squad_ready_path;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
      if (!AudioPlayer::instance().UpdateSquadReady(
              settings.settings.squad_ready_path.value_or(""))) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                           "Failed to load file");
      } else {
        AudioPlayer::instance().PlaySquadReady();
      }
    }
  }

  if (ImGui::Button("Play Squad Ready")) {
    if (!AudioPlayer::instance().UpdateSquadReady(
            settings.settings.squad_ready_path.value_or(""))) {
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to load file");
    }
  }

  ImGui::Text("Unofficial extras status: ");
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(
        "Unofficial extras is required for receiving squad member updates.");
  }
  ImGui::SameLine();
  if (globals::unofficial_extras_loaded) {
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Loaded");
  } else {
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Not Loaded");
  }
}
