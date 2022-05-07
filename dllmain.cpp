#include <Windows.h>
#include <stdint.h>
#include <stdio.h>

#include <map>
#include <mutex>

#include "Logging.h"
#include "arcdps-extension/arcdps_structs.h"
#include "arcdps_unofficial_extras_releases/Definitions.h"
#include "resource.h"

#pragma comment(lib, "winmm.lib")

const char* SQUAD_READY_PLUGIN_NAME = "squad_ready";
const char* SQUAD_READY_PLUGIN_VERSION = "0.1";

/* proto/globals */
HMODULE self_dll;
arcdps_exports arc_exports = {};
char* arcvers;
std::string selfAccountName;
std::map<std::string, UserInfo> cachedPlayers;
std::mutex cachedPlayersMutex;

/* arcdps exports */
arc_export_func_u64 ARC_EXPORT_E6;
arc_export_func_u64 ARC_EXPORT_E7;
e3_func_ptr ARC_LOG_FILE;
e3_func_ptr ARC_LOG;

bool initFailed = false;
bool extrasLoaded = false;
bool inReadyCheck = false;

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

  memset(&arc_exports, 0, sizeof(arcdps_exports));
  arc_exports.sig = 0xFFFA;
  arc_exports.imguivers = 18000;
  arc_exports.size = sizeof(arcdps_exports);
  arc_exports.out_name = SQUAD_READY_PLUGIN_NAME;
  arc_exports.out_build = SQUAD_READY_PLUGIN_VERSION;

  log_debug("done mod_init");
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

void ready_check_started() {
  log_debug("ready check has started");
  inReadyCheck = true;
  PlaySound(MAKEINTRESOURCE(READY_CHECK), self_dll, SND_RESOURCE | SND_ASYNC);
}

void ready_check_completed() {
  log_debug("squad is ready");
  PlaySound(MAKEINTRESOURCE(SQUAD_READY), self_dll, SND_RESOURCE | SND_ASYNC);
}

void ready_check_ended() {
  log_debug("ready check has ended");
  inReadyCheck = false;
}

bool all_players_readied() {
  // iterate through all cachedPlayers, and check readyStatus == true and
  // in squad
  for (auto const& [accountName, user] : cachedPlayers) {
    if (user.Role != UserRole::SquadLeader &&
        user.Role != UserRole::Lieutenant && user.Role != UserRole::Member) {
      log_debug(std::format("ignoring {} because they are role {}", accountName,
                            (int)user.Role));
      continue;
    }
    if (!user.ReadyStatus) {
      log_debug(std::format("squad not ready due to {}", accountName));
      return false;
    }
  }
  log_debug("all players are readied");
  return true;
}

/*
 * Ready check behaviour:
 * - When squad leader starts a ready check, their ReadyStatus switches from
 * false to true.
 * - If squad leader's ReadyStatus switches from true to false, this can be:
 *   - Ready check was cancelled.
 *   - Ready check succeeded, and everyone is being "unreadied".
 * - When a ready check succeeds, ordering of being "unreadied" cannot be relied
 *   upon to detect.
 */
void squad_update_callback(const UserInfo* updatedUsers,
                           size_t updatedUsersCount) {
  std::scoped_lock<std::mutex> guard(cachedPlayersMutex);
  log_debug(
      std::format("received squad callback with {} users", updatedUsersCount));
  for (size_t i = 0; i < updatedUsersCount; i++) {
    const auto user = updatedUsers[i];
    auto userAccountName = std::string(user.AccountName);
    if (userAccountName.at(0) == ':') {
      userAccountName.erase(0, 1);
    }
    log_debug(
        std::format("updated user {} accountname: {} ready: {} role: {} "
                    "jointime: {} subgroup: {}",
                    i, user.AccountName, user.ReadyStatus, (uint8_t)user.Role,
                    user.JoinTime, user.Subgroup));
    // User added/updated
    if (user.Role != UserRole::None) {
      auto oldUserIt = cachedPlayers.find(userAccountName);
      if (oldUserIt == cachedPlayers.end()) {
        // User added
        cachedPlayers.emplace(userAccountName, user);
      } else {
        // User updated
        auto oldUser = oldUserIt->second;
        cachedPlayers.insert_or_assign(userAccountName, user);

        if (user.Role == UserRole::SquadLeader) {
          if (user.ReadyStatus && !oldUser.ReadyStatus) {
            // Squad leader has started a ready check
            ready_check_started();
          } else {
            // Squad leader has ended a ready check, either via a complete ready
            // check or by cancelling
            ready_check_ended();
          }
        } else {
          if (all_players_readied()) {
            ready_check_completed();
          }
        }
      }
    }
    // User removed
    else {
      if (userAccountName == selfAccountName) {
        // Self left squad, reset cache
        cachedPlayers.clear();
      } else {
        // Remove player from cache
        cachedPlayers.erase(userAccountName);
      }
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

void update_self_user(std::string name) {
  if (name.at(0) == ':') name.erase(0, 1);

  if (selfAccountName.empty()) {
    selfAccountName = name;
  }
}

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

    update_self_user(pExtrasInfo->SelfAccountName);

    extrasSubscriberInfo->SubscriberName = SQUAD_READY_PLUGIN_NAME;
    extrasSubscriberInfo->SquadUpdateCallback = squad_update_callback;
  }
  // MaxInfoVersion has to be higher to have enough space to hold this object
  else if (pExtrasInfo->ApiVersion == 2 && pExtrasInfo->MaxInfoVersion >= 1) {
    ExtrasSubscriberInfoV1* subscriberInfo =
        static_cast<ExtrasSubscriberInfoV1*>(pSubscriberInfo);

    update_self_user(pExtrasInfo->SelfAccountName);

    subscriberInfo->InfoVersion = 1;
    subscriberInfo->SubscriberName = SQUAD_READY_PLUGIN_NAME;
    subscriberInfo->SquadUpdateCallback = squad_update_callback;
  }
  log_squad("done arcdps_unofficial_extras_subscriber_init");
}
