#pragma once

#include <format>
#include <string>

#include "extension/arcdps_structs.h"

namespace logging {
/* log to arcdps.log, thread/async safe */
void File(char* str);
/* log to extensions tab in arcdps log window, thread/async safe */
void Arc(char* str);

void Squad(char* str);
void Squad(const char* str);
void Squad(std::string str);

void Debug(char* str);
void Debug(const char* str);
void Debug(std::string str);
}  // namespace logging
