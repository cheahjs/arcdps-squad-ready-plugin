#include "Logging.h"

void logging::File(char* str) {
  if (ARC_LOG_FILE) ARC_LOG_FILE(str);
  return;
}

/* log to extensions tab in arcdps log window, thread/async safe */
void logging::Arc(char* str) {
  if (ARC_LOG) ARC_LOG(str);
  return;
}

void logging::Squad(char* str) {
#if _DEBUG
  logging::Arc((char*)std::format("squad_ready: {}", str).c_str());
#endif
  logging::File((char*)std::format("squad_ready: {}", str).c_str());
  return;
}

void logging::Squad(const char* str) {
  logging::Squad((char*)str);
  return;
}

void logging::Squad(std::string str) {
  logging::Squad((char*)str.c_str());
  return;
}

void logging::Debug(char* str) {
#if _DEBUG
  logging::Squad(std::format("DEBUG: {}", str).c_str());
#endif
  return;
}

void logging::Debug(const char* str) {
#if _DEBUG
  logging::Debug((char*)str);
#endif return;
}

void logging::Debug(std::string str) {
#if _DEBUG
  logging::Debug((char*)str.c_str());
#endif
  return;
}
