#pragma once

#include <Windows.h>

#include <string>

#include "Logging.h"
#include "extension/Singleton.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/extras/miniaudio_split/miniaudio.h"

class WaveFile {
 public:
  WaveFile();
  WaveFile(const std::string& file_name, ma_engine* engine);
  WaveFile(LPWSTR resource, ma_engine* engine);
  ~WaveFile();
  void Play() const;
  bool IsValid() const;
  void SetVolume(int volume) const;
  std::string ErrorMessage() const;

 private:
  char* buffer_;
  std::unique_ptr<ma_sound> sound_;
  std::unique_ptr<ma_decoder> decoder_;
  std::string error_message_ = "Unknown error";
  bool valid_;
};

class AudioPlayer final : public Singleton<AudioPlayer> {
 public:
  AudioPlayer() = default;
  ~AudioPlayer() override;

  bool Init(std::string ready_check_path, int ready_check_volume,
            std::string squad_ready_path, int squad_ready_volume);
  bool ReInit();
  void PlayReadyCheck() const;
  void PlaySquadReady() const;
  bool UpdateReadyCheck(const std::string& path);
  bool UpdateSquadReady(const std::string& path);
  void UpdateReadyCheckVolume(int volume);
  void UpdateSquadReadyVolume(int volume);
  std::string ReadyCheckStatus();
  std::string SquadReadyStatus();
  std::string OutputDeviceName();

private:
  void Destroy();

  std::unique_ptr<WaveFile> ready_check_sound_;
  int ready_check_volume_;
  std::string ready_check_path_;
  std::unique_ptr<WaveFile> squad_ready_sound_;
  int squad_ready_volume_;
  std::string squad_ready_path_;
  std::unique_ptr<ma_engine> engine_;
  std::string ready_check_status_;
  std::string squad_ready_status_;
  std::string output_device_name_;
};
