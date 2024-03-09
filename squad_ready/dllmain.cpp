#include <Windows.h>
#include <cstdint>
#include <stdio.h>

#include <map>

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

const std::string kSquadReadyPluginName = "Squad Ready";
const std::string kSquadReadyReleaseUrl =
    "https://github.com/cheahjs/arcdps-squad-ready-plugin/releases/latest";

/* proto/globals */
arcdps_exports arc_exports = {};
char* arc_version;
std::unique_ptr<SquadTracker> squad_tracker;

/* arcdps exports */
arc_export_func_u64 ARC_EXPORT_E6;
arc_export_func_u64 ARC_EXPORT_E7;
e3_func_ptr ARC_LOG_FILE;
e3_func_ptr ARC_LOG;

bool init_failed = false;

/* dll attach -- from winapi */
void dll_init(HMODULE hModule) {
  globals::self_dll = hModule;
}

/* dll detach -- from winapi */
void dll_exit() { }

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
    case DLL_THREAD_DETACH:
    default:
      break;
  }
  return 1;
}

/* window callback -- return is assigned to umsg (return zero to not be
 * processed by arcdps or game) */
uintptr_t mod_wnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  try {
    globals::some_window = hWnd;

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
          io->KeysDown[globals::arc_global_mod1] = false;
          io->KeysDown[globals::arc_global_mod2] = false;
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
  SettingsUI::instance([](auto i) { i.Draw(squad_tracker); });

  return 0;
}

uintptr_t mod_windows(const char* windowname) {
  if (!windowname) {
  }
  return 0;
}

uintptr_t mod_imgui(uint32_t not_charsel_or_loading) {
  if (squad_tracker) {
    squad_tracker->Tick();
    squad_tracker->Draw();
  }
  UpdateChecker::instance([](auto i) {
    i.Draw(
        globals::update_state, kSquadReadyPluginName,
        "https://github.com/cheahjs/arcdps-squad-ready-plugin/releases/latest");
  });

  return 0;
}

/* initialize mod -- return table that arcdps will use for callbacks. exports
 * struct and strings are copied to arcdps memory only once at init */
arcdps_exports* mod_init() {
  bool loading_successful = true;
  std::string error_message = "Unknown error";

  const auto& current_version =
      UpdateChecker::instance().GetCurrentVersion(globals::self_dll);

  try {
    UpdateChecker::instance().ClearFiles(globals::self_dll);

    if (current_version) {
      globals::update_state =
          std::move(UpdateChecker::instance().CheckForUpdate(
              globals::self_dll, current_version.value(),
              "cheahjs/arcdps-squad-ready-plugin", false));
    }
    SettingsUI::instance(std::make_unique<SettingsUI>());
    Settings::instance(std::make_unique<Settings>()).load();
    AudioPlayer::instance(std::make_unique<AudioPlayer>())
        .Init(
        Settings::instance().settings.ready_check_path.value_or(""),
        Settings::instance().settings.ready_check_volume,
        Settings::instance().settings.squad_ready_path.value_or(""),
        Settings::instance().settings.squad_ready_volume);
    squad_tracker = std::make_unique<SquadTracker>();
  } catch (const std::exception& e) {
    loading_successful = false;
    error_message = "Error loading: ";
    error_message.append(e.what());
  }

  memset(&arc_exports, 0, sizeof(arcdps_exports));
  arc_exports.imguivers = IMGUI_VERSION_NUM;
  arc_exports.out_name = kSquadReadyPluginName.c_str();
  const std::string& version =
      current_version ? UpdateChecker::instance().GetVersionAsString(
                            current_version.value())
                      : "Unknown";
  const auto version_c_str = new char[version.length() + 1];
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
    auto buffer =
        new char[error_message.length() + 1];  // we need extra char for NUL
    memcpy(buffer, error_message.c_str(), size + 1);
    arc_exports.size = reinterpret_cast<uintptr_t>(buffer);
  }

  logging::Debug("done mod_init");
  return &arc_exports;
}

/* release mod -- return ignored */
uintptr_t mod_release() {
  logging::Debug("shutdown called");
  if (globals::update_state) {
    globals::update_state->FinishPendingTasks();
    globals::update_state.reset(nullptr);
  }

  Settings::instance([](Settings& i) { i.unload(); });

  g_singletonManagerInstance.Shutdown();

  squad_tracker.reset();

  return 0;
}

/* export -- arcdps looks for this exported function and calls the address it
 * returns on client load */
extern "C" __declspec(dllexport) void* get_init_addr(
    char* arcversion, ImGuiContext* imguicontext, void* id3dptr, HMODULE arcdll,
    void* mallocfn, void* freefn, uint32_t d3dversion) {
  // id3dptr is IDirect3D9* if d3dversion==9, or IDXGISwapChain* if
  // d3dversion==11
  arc_version = arcversion;
  ImGui::SetCurrentContext(imguicontext);
  ImGui::SetAllocatorFunctions(static_cast<void*(*)(size_t, void*)>(mallocfn),
                               static_cast<void(*)(void*, void*)>(freefn));

  ARC_EXPORT_E6 = reinterpret_cast<arc_export_func_u64>(GetProcAddress(arcdll, "e6"));
  ARC_EXPORT_E7 = reinterpret_cast<arc_export_func_u64>(GetProcAddress(arcdll, "e7"));
  ARC_LOG_FILE = reinterpret_cast<e3_func_ptr>(GetProcAddress(arcdll, "e3"));
  ARC_LOG = reinterpret_cast<e3_func_ptr>(GetProcAddress(arcdll, "e8"));
  return mod_init;
}

/* export -- arcdps looks for this exported function and calls the address it
 * returns on client exit */
extern "C" __declspec(dllexport) void* get_release_addr() {
  arc_version = nullptr;
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
    const auto extras_subscriber_info =
        static_cast<ExtrasSubscriberInfo*>(pSubscriberInfo);

    globals::UpdateSelfUser(pExtrasInfo->SelfAccountName);

    extras_subscriber_info->SubscriberName = kSquadReadyPluginName.c_str();
    extras_subscriber_info->SquadUpdateCallback = squad_update_callback;
  }
  // MaxInfoVersion has to be higher to have enough space to hold this object
  else if (pExtrasInfo->ApiVersion == 2 && pExtrasInfo->MaxInfoVersion >= 1) {
    const auto subscriber_info =
        static_cast<ExtrasSubscriberInfoV1*>(pSubscriberInfo);

    globals::UpdateSelfUser(pExtrasInfo->SelfAccountName);

    subscriber_info->InfoVersion = 1;
    subscriber_info->SubscriberName = kSquadReadyPluginName.c_str();
    subscriber_info->SquadUpdateCallback = squad_update_callback;
  }
  logging::Debug("done arcdps_unofficial_extras_subscriber_init");
}
