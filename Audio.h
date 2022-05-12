#pragma once

#include <Windows.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Globals.h"
#include "Logging.h"
#include "resource.h"

#pragma comment(lib, "winmm.lib")

class WaveFile {
 public:
  WaveFile();
  WaveFile(std::string filename);
  WaveFile(LPWSTR resource);
  ~WaveFile();
  void Play();
  bool IsValid();

 private:
  char* buffer;
  bool valid;
};

class AudioPlayer {
 public:
  bool Init(std::string ready_check_path, std::string squad_ready_path);
  void PlayReadyCheck();
  void PlaySquadReady();
  bool UpdateReadyCheck(std::string path);
  bool UpdateSquadReady(std::string path);

 private:
  std::unique_ptr<WaveFile> ready_check_sound;
  std::unique_ptr<WaveFile> squad_ready_sound;
};
