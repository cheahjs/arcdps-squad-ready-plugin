#include "Settings.h"

#include <fstream>

#include "Logging.h"

void Settings::load() {
  // according to standard, this constructor is completely thread-safe
  // read settings from file
  ReadFromFile();
}

void Settings::unload() {
  try {
    SaveToFile();
  } catch (const std::exception& e) {
    logging::Squad("Failed to save settings");
    logging::Squad(e.what());
  }
}

void Settings::SaveToFile() {
  // create json object
  const auto json = nlohmann::json(settings);

  // open output file
  std::ofstream json_file(kSettingsJsonPath);

  // save json to file
  json_file << json;

  // close file
  json_file.close();
}

void Settings::ReadFromFile() {
  try {
    // read a JSON file as stream
    if (std::ifstream json_file(kSettingsJsonPath); json_file.is_open()) {
      nlohmann::json json;

      // push stream into json object (this also parses it)
      json_file >> json;

      // close file
      json_file.close();

      // get the object into the settings object
      json.get_to(settings);

      /**
       * MIGRATIONS
       */
    }
  } catch (const std::exception& e) {
    logging::Squad(
        "Failed to read settings, all settings have been reset to default");
    logging::Squad(e.what());
  }
}
