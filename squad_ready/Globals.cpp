#include "Globals.h"

#include "extension/arcdps_structs.h"
#include "imgui/imgui.h"

namespace globals {

std::string self_account_name;
HMODULE self_dll;
bool unofficial_extras_loaded;

// updates
std::unique_ptr<UpdateCheckerBase::UpdateState> UPDATE_STATE = nullptr;

// arc keyboard modifier
DWORD ARC_GLOBAL_MOD1 = 0;
DWORD ARC_GLOBAL_MOD2 = 0;
DWORD ARC_GLOBAL_MOD_MULTI = 0;

// Arc export Cache
bool ARC_HIDE_ALL = false;
bool ARC_PANEL_ALWAYS_DRAW = false;
bool ARC_MOVELOCK_ALTUI = false;
bool ARC_CLICKLOCK_ALTUI = false;
bool ARC_WINDOW_FASTCLOSE = false;

// Arc helper functions
void UpdateArcExports() {
  uint64_t e6_result = ARC_EXPORT_E6();
  uint64_t e7_result = ARC_EXPORT_E7();

  ARC_HIDE_ALL = (e6_result & 0x01);
  ARC_PANEL_ALWAYS_DRAW = (e6_result & 0x02);
  ARC_MOVELOCK_ALTUI = (e6_result & 0x04);
  ARC_CLICKLOCK_ALTUI = (e6_result & 0x08);
  ARC_WINDOW_FASTCLOSE = (e6_result & 0x10);

  uint16_t* ra = (uint16_t*)&e7_result;
  if (ra) {
    ARC_GLOBAL_MOD1 = ra[0];
    ARC_GLOBAL_MOD2 = ra[1];
    ARC_GLOBAL_MOD_MULTI = ra[2];
  }
}

bool ModsPressed() {
  auto io = &ImGui::GetIO();

  return io->KeysDown[ARC_GLOBAL_MOD1] && io->KeysDown[ARC_GLOBAL_MOD2];
}

bool CanMoveWindows() {
  if (!ARC_MOVELOCK_ALTUI) {
    return true;
  } else {
    return ModsPressed();
  }
}

void UpdateSelfUser(std::string name) {
  if (name.at(0) == ':') name.erase(0, 1);

  if (self_account_name.empty()) {
    self_account_name = name;
  }
}

}  // namespace globals
