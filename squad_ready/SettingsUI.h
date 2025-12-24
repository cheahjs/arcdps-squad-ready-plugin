#pragma once
#include "SquadTracker.h"
#include "extension/Singleton.h"

class SettingsUI : public ArcdpsExtension::Singleton<SettingsUI> {
 public:
  SettingsUI() = default;

  static void Draw(std::unique_ptr<SquadTracker>& tracker);
};
