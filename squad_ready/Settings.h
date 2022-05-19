#pragma once
#include <map>
#include <nlohmann/json.hpp>

#include "../modules/extension/Singleton.h"
#include "../modules/extension/arcdps_structs.h"
#include "../modules/extension/nlohmannJsonExtension.h"
#include "../modules/imgui/imgui.h"
#include "../modules/unofficial_extras/Definitions.h"

const std::string kSettingsJsonPath = "addons\\arcdps\\arcdps_squad_ready.json";

class Settings : public Singleton<Settings> {
 public:
  struct SettingsObject {
    uint32_t version = 1;
    std::optional<std::string> ready_check_path;
    std::optional<std::string> squad_ready_path;
    int ready_check_volume = 100;
    int squad_ready_volume = 100;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_NON_THROWING(SettingsObject,
                                                ready_check_path,
                                                squad_ready_path,
                                                ready_check_volume,
                                                squad_ready_volume)
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
  void saveToFile();
  void readFromFile();
};
