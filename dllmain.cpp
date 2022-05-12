#include <Windows.h>
#include <stdint.h>
#include <stdio.h>

#include <map>
#include <mutex>

#include "Globals.h"
#include "Logging.h"
#include "SquadTracker.h"
#include "Audio.h"
#include "arcdps-extension/arcdps_structs.h"
#include "arcdps_unofficial_extras_releases/Definitions.h"

#pragma comment(lib, "winmm.lib")

const char* SQUAD_READY_PLUGIN_NAME = "squad_ready";
const char* SQUAD_READY_PLUGIN_VERSION = "0.1";

/* proto/globals */
arcdps_exports arc_exports = {};
char* arcvers;
AudioPlayer* audio_player;
SquadTracker* squad_tracker;

/* arcdps exports */
arc_export_func_u64 ARC_EXPORT_E6;
arc_export_func_u64 ARC_EXPORT_E7;
e3_func_ptr ARC_LOG_FILE;
e3_func_ptr ARC_LOG;

bool init_failed = false;
bool extrasLoaded = false;

/* dll attach -- from winapi */
void dll_init(HMODULE hModule) {
  self_dll = hModule;
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

/* initialize mod -- return table that arcdps will use for callbacks. exports
 * struct and strings are copied to arcdps memory only once at init */
arcdps_exports* mod_init() {
  bool loading_successful = true;
  std::string error_message = "Unknown error";

  try {
    audio_player = new AudioPlayer();
    audio_player->Init("", "");
    squad_tracker = new SquadTracker(audio_player);
  } catch (const std::exception& e) {
    loading_successful = false;
    error_message = "Error loading: ";
    error_message.append(e.what());
  }

  memset(&arc_exports, 0, sizeof(arcdps_exports));
  arc_exports.imguivers = 18000;
  arc_exports.out_name = SQUAD_READY_PLUGIN_NAME;
  arc_exports.out_build = SQUAD_READY_PLUGIN_VERSION;

  if (loading_successful) {
    arc_exports.sig = 0xFFFA;
    arc_exports.size = sizeof(arcdps_exports);

  } else {
    init_failed = true;
    arc_exports.sig = 0;
    const std::string::size_type size = error_message.size();
    char* buffer = new char[size + 1];  // we need extra char for NUL
    memcpy(buffer, error_message.c_str(), size + 1);
    arc_exports.size = (uintptr_t)buffer;
  }

  logging::Debug("done mod_init");
  return &arc_exports;
}

/* release mod -- return ignored */
uintptr_t mod_release() {
  FreeConsole();
  return 0;
}

/* export -- arcdps looks for this exported function and calls the address it
 * returns on client load */
extern "C" __declspec(dllexport) void* get_init_addr(
    char* arcversion, void* imguictx, void* id3dptr, HMODULE arcdll,
    void* mallocfn, void* freefn, uint32_t d3dversion) {
  // id3dptr is IDirect3D9* if d3dversion==9, or IDXGISwapChain* if
  // d3dversion==11
  arcvers = arcversion;
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
  extrasLoaded = true;

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

    UpdateSelfUser(pExtrasInfo->SelfAccountName);

    extrasSubscriberInfo->SubscriberName = SQUAD_READY_PLUGIN_NAME;
    extrasSubscriberInfo->SquadUpdateCallback = squad_update_callback;
  }
  // MaxInfoVersion has to be higher to have enough space to hold this object
  else if (pExtrasInfo->ApiVersion == 2 && pExtrasInfo->MaxInfoVersion >= 1) {
    ExtrasSubscriberInfoV1* subscriberInfo =
        static_cast<ExtrasSubscriberInfoV1*>(pSubscriberInfo);

    UpdateSelfUser(pExtrasInfo->SelfAccountName);

    subscriberInfo->InfoVersion = 1;
    subscriberInfo->SubscriberName = SQUAD_READY_PLUGIN_NAME;
    subscriberInfo->SquadUpdateCallback = squad_update_callback;
  }
  logging::Squad("done arcdps_unofficial_extras_subscriber_init");
}
