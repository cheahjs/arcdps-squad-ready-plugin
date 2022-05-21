#pragma once
#include "extension/Singleton.h"

class SettingsUI : public Singleton<SettingsUI> {
 public:
  SettingsUI() = default;

  void Draw() const;
};
