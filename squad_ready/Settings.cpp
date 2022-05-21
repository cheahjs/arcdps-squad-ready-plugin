#include "Settings.h"

#include <fstream>

void Settings::load() {
  // according to standard, this constructor is completely thread-safe
  // read settings from file
  ReadFromFile();
}

void Settings::unload() {
  try {
    SaveToFile();
  } catch (const std::exception& e) {
    // Some exception happened, i cannot do anything against it here :(
  }
}

void Settings::SaveToFile() {
  // create json object
  const auto json = nlohmann::json(settings);

  // open output file
  std::ofstream json_file(kSettingsJsonPath);

  // save json to file
  json_file << json;
}

void Settings::ReadFromFile() {
  try {
    // read a JSON file as stream
    if (std::ifstream json_file(kSettingsJsonPath); json_file.is_open()) {
      nlohmann::json json;

      // push stream into json object (this also parses it)
      json_file >> json;

      // get the object into the settings object
      json.get_to(settings);

      /**
       * MIGRATIONS
       */
    }
  } catch (const std::exception& e) {
    // some exception was thrown, all settings are reset!
  }
}
