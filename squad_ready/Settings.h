#pragma once
#include <map>
#include <nlohmann/json.hpp>

#include "extension/Singleton.h"
#include "extension/arcdps_structs.h"
#include "extension/nlohmannJsonExtension.h"
#include "imgui/imgui.h"
#include "unofficial_extras/Definitions.h"

const std::string kSettingsJsonPath = "addons\\arcdps\\arcdps_squad_ready.json";

class Settings final : public Singleton<Settings> {
 public:
  struct SettingsObject {
    uint32_t version = 1;
    std::optional<std::string> ready_check_path;
    std::optional<std::string> squad_ready_path;
    int ready_check_volume = 100;
    int squad_ready_volume = 100;
    bool flash_window = true;
    bool ready_check_nag = false;
    bool ready_check_nag_in_combat = false;
    float ready_check_nag_interval_seconds = 5.0f;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_NON_THROWING(SettingsObject,
                                                ready_check_path,
                                                squad_ready_path,
                                                ready_check_volume,
                                                squad_ready_volume,
                                                flash_window,
                                                ready_check_nag,
                                                ready_check_nag_in_combat,
                                                ready_check_nag_interval_seconds)
  };

  Settings() = default;

  void load();
  void unload();

  SettingsObject settings;

  // delete copy/move
  Settings(const Settings& other) = delete;
  Settings(Settings&& other) noexcept = delete;
  Settings& operator=(const Settings& other) = delete;
  Settings& operator=(Settings&& other) noexcept = delete;

 private:
  void SaveToFile();
  void ReadFromFile();
};
