#include "Globals.h"

#include "extension/arcdps_structs.h"
#include "imgui/imgui.h"

namespace globals {

std::string self_account_name;
HMODULE self_dll;
HWND some_window = nullptr;
bool unofficial_extras_loaded;

// updates
std::unique_ptr<ArcdpsExtension::UpdateCheckerBase::UpdateState> update_state = nullptr;

// arc keyboard modifier
DWORD arc_global_mod1 = 0;
DWORD arc_global_mod2 = 0;
DWORD arc_global_mod_multi = 0;

// Arc export Cache
bool arc_hide_all = false;
bool arc_panel_always_draw = false;
bool arc_movelock_altui = false;
bool arc_clicklock_altui = false;
bool arc_window_fastclose = false;

// Arc helper functions
void UpdateArcExports() {
  const uint64_t e6_result = ARC_EXPORT_E6();
  uint64_t e7_result = ARC_EXPORT_E7();

  arc_hide_all = (e6_result & 0x01);
  arc_panel_always_draw = (e6_result & 0x02);
  arc_movelock_altui = (e6_result & 0x04);
  arc_clicklock_altui = (e6_result & 0x08);
  arc_window_fastclose = (e6_result & 0x10);

  if (auto ra = reinterpret_cast<uint16_t*>(&e7_result)) {
    arc_global_mod1 = ra[0];
    arc_global_mod2 = ra[1];
    arc_global_mod_multi = ra[2];
  }
}

bool ModsPressed() {
  const auto io = &ImGui::GetIO();

  return io->KeysDown[arc_global_mod1] && io->KeysDown[arc_global_mod2];
}

bool CanMoveWindows() {
  if (!arc_movelock_altui) {
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
