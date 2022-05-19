#include <Windows.h>
#include <stdint.h>
#include <stdio.h>

#include <map>
#include <mutex>

#include "Audio.h"
#include "Globals.h"
#include "Logging.h"
#include "Settings.h"
#include "SettingsUI.h"
#include "SquadTracker.h"
#include "extension/KeyBindHandler.h"
#include "extension/KeyInput.h"
#include "extension/Singleton.h"
#include "extension/UpdateCheckerBase.h"
#include "extension/Widgets.h"
#include "extension/Windows/PositioningComponent.h"
#include "extension/arcdps_structs.h"
#include "imgui/imgui.h"
#include "unofficial_extras/Definitions.h"

const char* SQUAD_READY_PLUGIN_NAME = "squad_ready";

/* proto/globals */
arcdps_exports arc_exports = {};
char* arcvers;
SquadTracker* squad_tracker;
UpdateCheckerBase* update_checker;

/* arcdps exports */
arc_export_func_u64 ARC_EXPORT_E6;
arc_export_func_u64 ARC_EXPORT_E7;
e3_func_ptr ARC_LOG_FILE;
e3_func_ptr ARC_LOG;

bool init_failed = false;

/* dll attach -- from winapi */
void dll_init(HMODULE hModule) {
  globals::self_dll = hModule;
  return;
}

/* dll detach -- from winapi */
void dll_exit() { return; }

/* dll main -- winapi */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReasonForCall,
                      LPVOID lpReserved) {
  switch (ulReasonForCall) {
    case DLL_PROCESS_ATTACH:
      dll_init(hModule);
      break;
    case DLL_PROCESS_DETACH:
      dll_exit();
      break;

    case DLL_THREAD_ATTACH:
      break;
    case DLL_THREAD_DETACH:
      break;
  }
  return 1;
}

/* window callback -- return is assigned to umsg (return zero to not be
 * processed by arcdps or game) */
uintptr_t mod_wnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  try {
    if (ImGuiEx::KeyCodeInputWndHandle(hWnd, uMsg, wParam, lParam)) {
      return 0;
    }

    if (KeyBindHandler::instance().Wnd(hWnd, uMsg, wParam, lParam)) {
      return 0;
    }

    auto const io = &ImGui::GetIO();

    switch (uMsg) {
      case WM_KEYUP:
      case WM_SYSKEYUP: {
        const int vkey = (int)wParam;
        io->KeysDown[vkey] = false;
        if (vkey == VK_CONTROL) {
          io->KeyCtrl = false;
        } else if (vkey == VK_MENU) {
          io->KeyAlt = false;
        } else if (vkey == VK_SHIFT) {
          io->KeyShift = false;
        }
        break;
      }
      case WM_KEYDOWN:
      case WM_SYSKEYDOWN: {
        const int vkey = (int)wParam;
        if (vkey == VK_CONTROL) {
          io->KeyCtrl = true;
        } else if (vkey == VK_MENU) {
          io->KeyAlt = true;
        } else if (vkey == VK_SHIFT) {
          io->KeyShift = true;
        }
        io->KeysDown[vkey] = true;
        break;
      }
      case WM_ACTIVATEAPP: {
        globals::UpdateArcExports();
        if (!wParam) {
          io->KeysDown[globals::ARC_GLOBAL_MOD1] = false;
          io->KeysDown[globals::ARC_GLOBAL_MOD2] = false;
        }
        break;
      }
      default:
        break;
    }
  } catch (const std::exception& e) {
    ARC_LOG_FILE("exception in mod_wnd");
    ARC_LOG_FILE(e.what());
    throw e;
  }
  return uMsg;
}

uintptr_t mod_options() {
  settingsUI.Draw();

  return 0;
}

uintptr_t mod_windows(const char* windowname) {
  if (!windowname) {
  }
  return 0;
}

bool lastFrameShow = false;

uintptr_t mod_imgui(uint32_t not_charsel_or_loading) { return 0; }

/* initialize mod -- return table that arcdps will use for callbacks. exports
 * struct and strings are copied to arcdps memory only once at init */
arcdps_exports* mod_init() {
  bool loading_successful = true;
  std::string error_message = "Unknown error";

  update_checker = new UpdateCheckerBase();
  const auto& currentVersion =
      update_checker->GetCurrentVersion(globals::self_dll);

  try {
    Settings::instance().load();
    AudioPlayer::instance().Init(
        Settings::instance().settings.ready_check_path.value_or(""),
        Settings::instance().settings.ready_check_volume,
        Settings::instance().settings.squad_ready_path.value_or(""),
        Settings::instance().settings.squad_ready_volume);
    squad_tracker = new SquadTracker();
  } catch (const std::exception& e) {
    loading_successful = false;
    error_message = "Error loading: ";
    error_message.append(e.what());
  }

  memset(&arc_exports, 0, sizeof(arcdps_exports));
  arc_exports.imguivers = IMGUI_VERSION_NUM;
  arc_exports.out_name = SQUAD_READY_PLUGIN_NAME;
  const std::string& version =
      currentVersion
          ? update_checker->GetVersionAsString(currentVersion.value())
          : "Unknown";
  char* version_c_str = new char[version.length() + 1];
  strcpy_s(version_c_str, version.length() + 1, version.c_str());
  arc_exports.out_build = version_c_str;

  if (loading_successful) {
    arc_exports.sig = 0xBCAB9171;
    arc_exports.size = sizeof(arcdps_exports);
    arc_exports.wnd_nofilter = mod_wnd;
    arc_exports.imgui = mod_imgui;
    arc_exports.options_end = mod_options;
    arc_exports.options_windows = mod_windows;
  } else {
    init_failed = true;
    arc_exports.sig = 0;
    const std::string::size_type size = error_message.size();
    char* buffer =
        new char[error_message.length() + 1];  // we need extra char for NUL
    memcpy(buffer, error_message.c_str(), size + 1);
    arc_exports.size = (uintptr_t)buffer;
  }

  logging::Debug("done mod_init");
  return &arc_exports;
}

/* release mod -- return ignored */
uintptr_t mod_release() {
  FreeConsole();

  Settings::instance().unload();

  g_singletonManagerInstance.Shutdown();

  return 0;
}

/* export -- arcdps looks for this exported function and calls the address it
 * returns on client load */
extern "C" __declspec(dllexport) void* get_init_addr(
    char* arcversion, ImGuiContext* imguicontext, void* id3dptr, HMODULE arcdll,
    void* mallocfn, void* freefn, uint32_t d3dversion) {
  // id3dptr is IDirect3D9* if d3dversion==9, or IDXGISwapChain* if
  // d3dversion==11
  arcvers = arcversion;
  ImGui::SetCurrentContext(imguicontext);
  ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))mallocfn,
                               (void (*)(void*, void*))freefn);

  ARC_EXPORT_E6 = (arc_export_func_u64)GetProcAddress(arcdll, "e6");
  ARC_EXPORT_E7 = (arc_export_func_u64)GetProcAddress(arcdll, "e7");
  ARC_LOG_FILE = (e3_func_ptr)GetProcAddress(arcdll, "e3");
  ARC_LOG = (e3_func_ptr)GetProcAddress(arcdll, "e8");
  return mod_init;
}

/* export -- arcdps looks for this exported function and calls the address it
 * returns on client exit */
extern "C" __declspec(dllexport) void* get_release_addr() {
  arcvers = nullptr;
  return mod_release;
}

/**
 * here for backwardscompatibility to unofficial extras API v1.
 * Can be removed after it has a proper release.
 */
typedef void (*SquadUpdateCallbackSignature)(const UserInfo* pUpdatedUsers,
                                             uint64_t pUpdatedUsersCount);
struct ExtrasSubscriberInfo {
  // Null terminated name of the addon subscribing to the changes. Must be valid
  // for the lifetime of the subcribing addon. Set to nullptr if initialization
  // fails
  const char* SubscriberName = nullptr;

  // Called whenever anything in the squad changes. Only the users that changed
  // are sent. If a user is removed from the squad, it will be sent with Role ==
  // UserRole::None
  SquadUpdateCallbackSignature SquadUpdateCallback = nullptr;
};

void squad_update_callback(const UserInfo* updatedUsers,
                           size_t updatedUsersCount) {
  if (init_failed) return;
  if (!squad_tracker) return;
  squad_tracker->UpdateUsers(updatedUsers, updatedUsersCount);
}

extern "C" __declspec(dllexport) void arcdps_unofficial_extras_subscriber_init(
    const ExtrasAddonInfo* pExtrasInfo, void* pSubscriberInfo) {
  globals::unofficial_extras_loaded = true;

  // do not subscribe, if initialization called from arcdps failed.
  if (init_failed) {
    logging::Squad(
        "init failed, not continuing with "
        "arcdps_unofficial_extras_subscriber_init");
    return;
  }
  if (pExtrasInfo->ApiVersion == 1) {
    // V1 of the unofficial extras API, treat is as that!
    ExtrasSubscriberInfo* extrasSubscriberInfo =
        static_cast<ExtrasSubscriberInfo*>(pSubscriberInfo);

    globals::UpdateSelfUser(pExtrasInfo->SelfAccountName);

    extrasSubscriberInfo->SubscriberName = SQUAD_READY_PLUGIN_NAME;
    extrasSubscriberInfo->SquadUpdateCallback = squad_update_callback;
  }
  // MaxInfoVersion has to be higher to have enough space to hold this object
  else if (pExtrasInfo->ApiVersion == 2 && pExtrasInfo->MaxInfoVersion >= 1) {
    ExtrasSubscriberInfoV1* subscriberInfo =
        static_cast<ExtrasSubscriberInfoV1*>(pSubscriberInfo);

    globals::UpdateSelfUser(pExtrasInfo->SelfAccountName);

    subscriberInfo->InfoVersion = 1;
    subscriberInfo->SubscriberName = SQUAD_READY_PLUGIN_NAME;
    subscriberInfo->SquadUpdateCallback = squad_update_callback;
  }
  logging::Debug("done arcdps_unofficial_extras_subscriber_init");
}
