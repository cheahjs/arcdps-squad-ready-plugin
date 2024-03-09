#pragma once

#include <format>
#include <string>

#include "extension/arcdps_structs.h"
#include "miniaudio/extras/miniaudio_split/miniaudio.h"

namespace logging {
/* log to arcdps.log, thread/async safe */
void File(const char* str);
/* log to extensions tab in arcdps log window, thread/async safe */
void Arc(const char* str);

void Squad(char* str);
void Squad(const char* str);
void Squad(std::string str);

void MiniAudioError(ma_result result, char* str);
void MiniAudioError(ma_result result, const char* str);
void MiniAudioError(ma_result result, std::string str);

void Debug(char* str);
void Debug(const char* str);
void Debug(const std::string& str);
}  // namespace logging
