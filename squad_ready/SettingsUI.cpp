#include "SettingsUI.h"

#include <string>

#include "Audio.h"
#include "Globals.h"
#include "Settings.h"
#include "extension/KeyInput.h"
#include "extension/Widgets.h"
#include "extension/imgui_stdlib.h"
#include "imgui/imgui.h"

SettingsUI settingsUI;

void SettingsUI::Draw() {
  Settings& settings = Settings::instance();

  // Ready Check settings
  ImGui::Spacing();
  ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Ready Check");
  int& ready_check_volume = settings.settings.ready_check_volume;
  if (ImGui::SliderInt("Volume - Ready Check", &ready_check_volume, 0, 100,
                       "%d%%")) {
    AudioPlayer::instance().UpdateReadyCheckVolume(ready_check_volume);
  }

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
      AudioPlayer::instance().UpdateReadyCheck(
          settings.settings.ready_check_path.value_or(""));
    }
  }

  if (ImGui::Button("Play Ready Check")) {
    if (AudioPlayer::instance().UpdateReadyCheck(
            settings.settings.ready_check_path.value_or(""))) {
      AudioPlayer::instance().PlayReadyCheck();
    }
  }

  auto ready_check_status = AudioPlayer::instance().ReadyCheckStatus();
  if (!ready_check_status.empty()) {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                       ready_check_status.c_str());
  }

  // Squad Ready settings
  ImGui::Spacing();
  ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Squad Ready");

  int& squad_ready_volume = settings.settings.squad_ready_volume;
  if (ImGui::SliderInt("Volume - Squad Ready", &squad_ready_volume, 0, 100,
                       "%d%%")) {
    AudioPlayer::instance().UpdateSquadReadyVolume(squad_ready_volume);
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
      AudioPlayer::instance().UpdateSquadReady(
          settings.settings.squad_ready_path.value_or(""));
    }
  }

  if (ImGui::Button("Play Squad Ready")) {
    if (AudioPlayer::instance().UpdateSquadReady(
            settings.settings.squad_ready_path.value_or(""))) {
      AudioPlayer::instance().PlaySquadReady();
    }
  }
  auto squad_ready_status = AudioPlayer::instance().SquadReadyStatus();
  if (!squad_ready_status.empty()) {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                       squad_ready_status.c_str());
  }

  // Unofficial extras status
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

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
}
