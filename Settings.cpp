#include "Settings.h"

#include <fstream>

void Settings::load() {
  // according to standard, this constructor is completely thread-safe
  // read settings from file
  readFromFile();
}

void Settings::unload() {
  try {
    saveToFile();
  } catch (const std::exception& e) {
    // Some exception happened, i cannot do anything against it here :(
  }
}

void Settings::saveToFile() {
  // create json object
  auto json = nlohmann::json(settings);

  // open output file
  std::ofstream json_file(kSettingsJsonPath);

  // save json to file
  json_file << json;
}

void Settings::readFromFile() {
  try {
    // read a JSON file as stream
    std::ifstream json_file(kSettingsJsonPath);
    if (json_file.is_open()) {
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
