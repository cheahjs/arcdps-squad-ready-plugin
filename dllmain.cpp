#include <Windows.h>
#include <stdint.h>
#include <stdio.h>

#include <mutex>

#include "Logging.h"
#include "arcdps-extension/arcdps_structs.h"
#include "arcdps_unofficial_extras_releases/Definitions.h"

const char* SQUAD_READY_PLUGIN_NAME = "squad_ready";
const char* SQUAD_READY_PLUGIN_VERSION = "0.1";

/* proto/globals */
arcdps_exports arc_exports = {};
char* arcvers;
std::vector<std::string> trackedPlayers;
std::mutex trackedPlayersMutex;

/* arcdps exports */
arc_export_func_u64 ARC_EXPORT_E6;
arc_export_func_u64 ARC_EXPORT_E7;
e3_func_ptr ARC_LOG_FILE;
e3_func_ptr ARC_LOG;

bool initFailed = false;
bool extrasLoaded = false;

/* dll attach -- from winapi */
void dll_init(HANDLE hModule) { return; }

/* dll detach -- from winapi */
void dll_exit() { return; }

/* dll main -- winapi */
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ulReasonForCall,
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
  /* for arcdps */
  memset(&arc_exports, 0, sizeof(arcdps_exports));
  arc_exports.sig = 0xFFFA;
  arc_exports.imguivers = 18000;
  arc_exports.size = sizeof(arcdps_exports);
  arc_exports.out_name = SQUAD_READY_PLUGIN_NAME;
  arc_exports.out_build = SQUAD_READY_PLUGIN_VERSION;
  // arc_exports.size = (uintptr_t)"error message if you decide to not load, sig
  // must be 0";
  log_squad("done mod_init");  // if using vs2015+, project properties > c++ >
                               // conformance mode > permissive to avoid const
                               // to not const conversion error
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
  arcvers = 0;
  return mod_release;
}

void squad_update_callback(const UserInfo* updatedUsers,
                           size_t updatedUsersCount) {
  std::scoped_lock<std::mutex> guard(trackedPlayersMutex);
  log_squad(
      std::format("received squad callback with {} users", updatedUsersCount));
  for (size_t i = 0; i < updatedUsersCount; i++) {
    const auto user = updatedUsers[i];
    log_debug(
        std::format("updated user {} accountname: {} ready: {} role: {} "
                    "jointime: {} subgroup: {}",
                    i, user.AccountName, user.ReadyStatus, (uint8_t)user.Role,
                    user.JoinTime, user.Subgroup));
    // User added/updated
    if (user.Role != UserRole::None) {
    }
    // User removed
    else {
    }
  }
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

extern "C" __declspec(dllexport) void arcdps_unofficial_extras_subscriber_init(
    const ExtrasAddonInfo* pExtrasInfo, void* pSubscriberInfo) {
  extrasLoaded = true;

  // do not subscribe, if initialization called from arcdps failed.
  if (initFailed) {
    log_squad(
        "init failed, not continuing with "
        "arcdps_unofficial_extras_subscriber_init");
    return;
  }
  if (pExtrasInfo->ApiVersion == 1) {
    // V1 of the unofficial extras API, treat is as that!
    ExtrasSubscriberInfo* extrasSubscriberInfo =
        static_cast<ExtrasSubscriberInfo*>(pSubscriberInfo);

    extrasSubscriberInfo->SubscriberName = SQUAD_READY_PLUGIN_NAME;
    extrasSubscriberInfo->SquadUpdateCallback = squad_update_callback;
  }
  // MaxInfoVersion has to be higher to have enough space to hold this object
  else if (pExtrasInfo->ApiVersion == 2 && pExtrasInfo->MaxInfoVersion >= 1) {
    ExtrasSubscriberInfoV1* subscriberInfo =
        static_cast<ExtrasSubscriberInfoV1*>(pSubscriberInfo);

    subscriberInfo->InfoVersion = 1;
    subscriberInfo->SubscriberName = SQUAD_READY_PLUGIN_NAME;
    subscriberInfo->SquadUpdateCallback = squad_update_callback;
  }
  log_squad("done arcdps_unofficial_extras_subscriber_init");
}
