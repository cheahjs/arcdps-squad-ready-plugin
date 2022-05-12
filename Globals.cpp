#include "Globals.h"

std::string self_account_name;
HMODULE self_dll;

void UpdateSelfUser(std::string name) {
  if (name.at(0) == ':') name.erase(0, 1);

  if (self_account_name.empty()) {
    self_account_name = name;
  }
}
