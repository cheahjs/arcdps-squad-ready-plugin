#pragma once

#include <Windows.h>

#include <string>

extern std::string self_account_name;
extern HMODULE self_dll;

void UpdateSelfUser(std::string name);
