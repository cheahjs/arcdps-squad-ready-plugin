#include "Logging.h"

void log_file(char* str) {
  if (ARC_LOG_FILE) ARC_LOG_FILE(str);
  return;
}

/* log to extensions tab in arcdps log window, thread/async safe */
void log_arc(char* str) {
  if (ARC_LOG) ARC_LOG(str);
  return;
}

void log_all(char* str) {
#if _DEBUG
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

void log_debug(char* str) {
#if _DEBUG
  log_squad(std::format("DEBUG: {}", str).c_str());
#endif
  return;
}

void log_debug(const char* str) {
#if _DEBUG
  log_debug((char*)str);
#endif return;
}

void log_debug(std::string str) {
#if _DEBUG
  log_debug((char*)str.c_str());
#endif
  return;
}
