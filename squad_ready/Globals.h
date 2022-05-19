#pragma once

#include <Windows.h>

#include <string>

namespace globals {

extern std::string self_account_name;
extern HMODULE self_dll;
extern bool unofficial_extras_loaded;

// Updating myself stuff
// extern std::unique_ptr<UpdateChecker> UPDATE_CHECKER = nullptr;
// extern std::unique_ptr<UpdateCheckerBase::UpdateState> UPDATE_STATE =
//    nullptr;

// arc keyboard modifier
extern DWORD ARC_GLOBAL_MOD1;
extern DWORD ARC_GLOBAL_MOD2;
extern DWORD ARC_GLOBAL_MOD_MULTI;

// Arc export Cache
extern bool ARC_HIDE_ALL;
extern bool ARC_PANEL_ALWAYS_DRAW;
extern bool ARC_MOVELOCK_ALTUI;
extern bool ARC_CLICKLOCK_ALTUI;
extern bool ARC_WINDOW_FASTCLOSE;

// Arc helper functions
void UpdateArcExports();
bool ModsPressed();
bool CanMoveWindows();

void UpdateSelfUser(std::string name);

}  // namespace globals
