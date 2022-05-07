#include <stdint.h>
#include <stdio.h>
#include <Windows.h>
#include <mutex>
#include <format>
#include <string>
#include <iostream>
#include <string_view>
#include "arcdps_unofficial_extras_releases/Definitions.h"

/* arcdps export table */
typedef struct arcdps_exports {
	uintptr_t size; /* size of exports table */
	uint32_t sig; /* pick a number between 0 and uint32_t max that isn't used by other modules */
	uint32_t imguivers; /* set this to IMGUI_VERSION_NUM. if you don't use imgui, 18000 (as of 2021-02-02) */
	const char* out_name; /* name string */
	const char* out_build; /* build string */
	void* wnd_nofilter; /* wndproc callback, fn(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam), return assigned to umsg */
	void* combat; /* combat event callback, fn(cbtevent* ev, ag* src, ag* dst, char* skillname, uint64_t id, uint64_t revision) */
	void* imgui; /* ::present callback, before imgui::render, fn(uint32_t not_charsel_or_loading) */
	void* options_end; /* ::present callback, appending to the end of options window in arcdps, fn() */
	void* combat_local;  /* combat event callback like area but from chat log, fn(cbtevent* ev, ag* src, ag* dst, char* skillname, uint64_t id, uint64_t revision) */
	void* wnd_filter; /* wndproc callback like wnd_nofilter above, input filered using modifiers */
	void* options_windows; /* called once per 'window' option checkbox, with null at the end, non-zero return disables arcdps drawing that checkbox, fn(char* windowname) */
} arcdps_exports;

/* combat event - see evtc docs for details, revision param in combat cb is equivalent of revision byte header */
typedef struct cbtevent {
	uint64_t time;
	uint64_t src_agent;
	uint64_t dst_agent;
	int32_t value;
	int32_t buff_dmg;
	uint32_t overstack_value;
	uint32_t skillid;
	uint16_t src_instid;
	uint16_t dst_instid;
	uint16_t src_master_instid;
	uint16_t dst_master_instid;
	uint8_t iff;
	uint8_t buff;
	uint8_t result;
	uint8_t is_activation;
	uint8_t is_buffremove;
	uint8_t is_ninety;
	uint8_t is_fifty;
	uint8_t is_moving;
	uint8_t is_statechange;
	uint8_t is_flanking;
	uint8_t is_shields;
	uint8_t is_offcycle;
	uint8_t pad61;
	uint8_t pad62;
	uint8_t pad63;
	uint8_t pad64;
} cbtevent;

/* agent short */
typedef struct ag {
	char* name; /* agent name. may be null. valid only at time of event. utf8 */
	uintptr_t id; /* agent unique identifier */
	uint32_t prof; /* profession at time of event. refer to evtc notes for identification */
	uint32_t elite; /* elite spec at time of event. refer to evtc notes for identification */
	uint32_t self; /* 1 if self, 0 if not */
	uint16_t team; /* sep21+ */
} ag;

const char* SQUAD_READY_PLUGIN_NAME = "squad_ready";
const char* SQUAD_READY_PLUGIN_VERSION = "0.1";

/* proto/globals */
uint32_t cbtcount = 0;
arcdps_exports arc_exports;
char* arcvers;
void dll_init(HANDLE hModule);
void dll_exit();
extern "C" __declspec(dllexport) void* get_init_addr(char* arcversion, void* imguictx, void* id3dptr, HANDLE arcdll, void* mallocfn, void* freefn, uint32_t d3dversion);
extern "C" __declspec(dllexport) void* get_release_addr();
arcdps_exports* mod_init();
uintptr_t mod_release();
void log_file(char* str);
void log_arc(char* str);

/* arcdps exports */
void* filelog;
void* arclog;

/* dll main -- winapi */
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ulReasonForCall, LPVOID lpReserved) {
	switch (ulReasonForCall) {
	case DLL_PROCESS_ATTACH: dll_init(hModule); break;
	case DLL_PROCESS_DETACH: dll_exit(); break;

	case DLL_THREAD_ATTACH:  break;
	case DLL_THREAD_DETACH:  break;
	}
	return 1;
}

/* log to arcdps.log, thread/async safe */
void log_file(char* str) {
	size_t(*log)(char*) = (size_t(*)(char*))filelog;
	if (log) (*log)(str);
	return;
}

/* log to extensions tab in arcdps log window, thread/async safe */
void log_arc(char* str) {
	size_t(*log)(char*) = (size_t(*)(char*))arclog;
	if (log) (*log)(str);
	return;
}

void log_all(char* str) {
#if DEBUG
	log_arc(str);
#endif
	log_file(str);
	return;
}

void log_squad(char* str) {
	log_all((char*)std::format("squad_ready: {}", str).c_str());
	return;
}

void log_squad(const char* str) {
	log_squad((char*)str);
	return;
}

void log_squad(std::string str) {
	log_squad((char*)str.c_str());
	return;
}

/* dll attach -- from winapi */
void dll_init(HANDLE hModule) {
	return;
}

/* dll detach -- from winapi */
void dll_exit() {
	return;
}

/* export -- arcdps looks for this exported function and calls the address it returns on client load */
extern "C" __declspec(dllexport) void* get_init_addr(char* arcversion, void* imguictx, void* id3dptr, HANDLE arcdll, void* mallocfn, void* freefn, uint32_t d3dversion) {
	// id3dptr is IDirect3D9* if d3dversion==9, or IDXGISwapChain* if d3dversion==11
	arcvers = arcversion;
	filelog = (void*)GetProcAddress((HMODULE)arcdll, "e3");
	arclog = (void*)GetProcAddress((HMODULE)arcdll, "e8");
	return mod_init;
}

/* export -- arcdps looks for this exported function and calls the address it returns on client exit */
extern "C" __declspec(dllexport) void* get_release_addr() {
	arcvers = 0;
	return mod_release;
}

/* initialize mod -- return table that arcdps will use for callbacks. exports struct and strings are copied to arcdps memory only once at init */
arcdps_exports* mod_init() {
	/* for arcdps */
	memset(&arc_exports, 0, sizeof(arcdps_exports));
	arc_exports.sig = 0xFFFA;
	arc_exports.imguivers = 18000;
	arc_exports.size = sizeof(arcdps_exports);
	arc_exports.out_name = SQUAD_READY_PLUGIN_NAME;
	arc_exports.out_build = SQUAD_READY_PLUGIN_VERSION;
	//arc_exports.size = (uintptr_t)"error message if you decide to not load, sig must be 0";
	log_squad("done mod_init"); // if using vs2015+, project properties > c++ > conformance mode > permissive to avoid const to not const conversion error
	return &arc_exports;
}

/* release mod -- return ignored */
uintptr_t mod_release() {
	FreeConsole();
	return 0;
}

void squad_update_callback(const UserInfo* updatedUsers, size_t updatedUsersCount) {
	log_squad(std::format("received squad callback with {} users", updatedUsersCount));
	for (size_t i = 0; i < updatedUsersCount; i++) {
		const auto user = updatedUsers[i];
		log_squad(std::format("updated user {} accountname: {} ready: {} role: {} jointime: {} subgroup: {}", i, user.AccountName, user.ReadyStatus, (uint8_t)user.Role, user.JoinTime, user.Subgroup));
	}
}

/**
 * here for backwardscompatibility to unofficial extras API v1.
 * Can be removed after it has a proper release.
 */
typedef void (*SquadUpdateCallbackSignature)(const UserInfo* pUpdatedUsers, uint64_t pUpdatedUsersCount);
struct ExtrasSubscriberInfo {
	// Null terminated name of the addon subscribing to the changes. Must be valid for the lifetime of the subcribing addon. Set to
	// nullptr if initialization fails
	const char* SubscriberName = nullptr;

	// Called whenever anything in the squad changes. Only the users that changed are sent. If a user is removed from
	// the squad, it will be sent with Role == UserRole::None
	SquadUpdateCallbackSignature SquadUpdateCallback = nullptr;
};

extern "C" __declspec(dllexport) void arcdps_unofficial_extras_subscriber_init(
	const ExtrasAddonInfo * pExtrasInfo, void* pSubscriberInfo) {
	if (pExtrasInfo->ApiVersion == 1) {
		// V1 of the unofficial extras API, treat is as that!
		ExtrasSubscriberInfo* extrasSubscriberInfo = static_cast<ExtrasSubscriberInfo*>(pSubscriberInfo);

		extrasSubscriberInfo->SubscriberName = SQUAD_READY_PLUGIN_NAME;
		extrasSubscriberInfo->SquadUpdateCallback = squad_update_callback;
	}
	// MaxInfoVersion has to be higher to have enough space to hold this object
	else if (pExtrasInfo->ApiVersion == 2 && pExtrasInfo->MaxInfoVersion >= 1) {
		ExtrasSubscriberInfoV1* subscriberInfo = static_cast<ExtrasSubscriberInfoV1*>(pSubscriberInfo);

		subscriberInfo->InfoVersion = 1;
		subscriberInfo->SubscriberName = SQUAD_READY_PLUGIN_NAME;
		subscriberInfo->SquadUpdateCallback = squad_update_callback;
	}
	log_squad("done arcdps_unofficial_extras_subscriber_init");
}
