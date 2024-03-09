#include "Logging.h"

void logging::File(const char* str) {
  if (ARC_LOG_FILE) ARC_LOG_FILE(str);
}

/* log to extensions tab in arcdps log window, thread/async safe */
void logging::Arc(const char* str) {
  if (ARC_LOG) ARC_LOG(str);
}

void logging::Squad(char* str) {
#if _DEBUG
  Arc(std::format("squad_ready: {}", str).c_str());
#endif
  File(std::format("squad_ready: {}", str).c_str());
}

void logging::Squad(const char* str) {
  Squad(const_cast<char*>(str));
}

void logging::Squad(std::string str) {
  Squad(const_cast<char*>(str.c_str()));
}

void logging::MiniAudioError(ma_result result, char* str) {
  if (result != MA_SUCCESS) {
    Squad(std::format("MiniAudio error: {} ({})", str, ma_result_description(result)).c_str());
  }
}

void logging::MiniAudioError(ma_result result, const char* str) {
  MiniAudioError(result, const_cast<char*>(str));
}

void logging::MiniAudioError(ma_result result, std::string str) {
  MiniAudioError(result, const_cast<char*>(str.c_str()));
}

void logging::Debug(char* str) {
#if _DEBUG
  Squad(std::format("DEBUG: {}", str).c_str());
#endif
}

void logging::Debug(const char* str) {
#if _DEBUG
  Debug(const_cast<char*>(str));
#endif
}

void logging::Debug(const std::string& str) {
#if _DEBUG
  Debug(const_cast<char*>(str.c_str()));
#endif
}
