#pragma once

#include <Windows.h>

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
