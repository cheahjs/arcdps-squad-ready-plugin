#pragma once

#include <Windows.h>

#include <array>
#include <string>

#include "extension/UpdateChecker.h"
#include "extension/UpdateCheckerBase.h"

namespace globals {

extern std::string self_account_name;
extern HMODULE self_dll;
extern HWND some_window;
extern bool unofficial_extras_loaded;

// Updating myself stuff
extern std::unique_ptr<UpdateCheckerBase::UpdateState> update_state;

// Windows VK state tracked from WndProc. Replaces the pre-1.87 io.KeysDown
// array that ImGui removed — arcdps dispatches the WndProc events to us and
// we track them ourselves since arc_global_mod1/2 are virtual-key codes.
extern std::array<bool, 256> vk_down;

// arc keyboard modifier
extern DWORD arc_global_mod1;
extern DWORD arc_global_mod2;
extern DWORD arc_global_mod_multi;

// Arc export Cache
extern bool arc_hide_all;
extern bool arc_panel_always_draw;
extern bool arc_movelock_altui;
extern bool arc_clicklock_altui;
extern bool arc_window_fastclose;

// Arc helper functions
void UpdateArcExports();
bool ModsPressed();
bool CanMoveWindows();

void UpdateSelfUser(std::string name);

}  // namespace globals
