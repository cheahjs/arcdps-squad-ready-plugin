#pragma once

#include <format>
#include <string>

#include "arcdps-extension/arcdps_structs.h"

/* log to arcdps.log, thread/async safe */
void log_file(char* str);
/* log to extensions tab in arcdps log window, thread/async safe */
void log_arc(char* str);

void log_squad(char* str);
void log_squad(const char* str);
void log_squad(std::string str);

void log_debug(char* str);
void log_debug(const char* str);
void log_debug(std::string str);
