#pragma once

#include <Windows.h>

#include <string>

#include "Logging.h"
#include "extension/Singleton.h"

#define MINIAUDIO_IMPLEMENTATION
#include "../modules/miniaudio/extras/miniaudio_split/miniaudio.h"

class WaveFile {
 public:
  WaveFile();
  WaveFile(std::string filename, ma_engine* engine);
  WaveFile(LPWSTR resource, ma_engine* engine);
  ~WaveFile();
  void Play();
  bool IsValid();
  void SetVolume(int volume);

 private:
  char* buffer;
  std::unique_ptr<ma_sound> sound;
  std::unique_ptr<ma_decoder> decoder;
  bool valid;
};

class AudioPlayer : public Singleton<AudioPlayer> {
 public:
  AudioPlayer() = default;
  ~AudioPlayer();

  bool Init(std::string ready_check_path, int ready_check_volume,
            std::string squad_ready_path, int squad_ready_volume);
  void PlayReadyCheck();
  void PlaySquadReady();
  bool UpdateReadyCheck(std::string path);
  bool UpdateSquadReady(std::string path);
  void UpdateReadyCheckVolume(int volume);
  void UpdateSquadReadyVolume(int volume);
  std::string ReadyCheckStatus();
  std::string SquadReadyStatus();

 private:
  std::unique_ptr<WaveFile> ready_check_sound;
  std::unique_ptr<WaveFile> squad_ready_sound;
  int ready_check_volume;
  int squad_ready_volume;
  std::unique_ptr<ma_engine> engine;
  std::string ready_check_status;
  std::string squad_ready_status;
};