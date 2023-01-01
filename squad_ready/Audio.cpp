#include "Audio.h"

#include <fstream>

#include "Error.h"
#include "Globals.h"
#include "resource.h"

AudioPlayer::~AudioPlayer() {
  Destroy();
}

bool AudioPlayer::Init(const std::string ready_check_path, const int ready_check_volume,
                       const std::string squad_ready_path, const int squad_ready_volume) {
  bool success = true;

  ready_check_path_ = ready_check_path;
  squad_ready_path_ = squad_ready_path;

  engine_ = std::make_unique<ma_engine>();
  if (ma_engine_init(nullptr, engine_.get()) != MA_SUCCESS) {
    logging::Squad("Failed to initialize audio engine");
    return false;
  }

  if (!UpdateReadyCheck(ready_check_path)) {
    logging::Squad(std::format("Failed to load ready check audio from {}: {}",
                               ready_check_path, ready_check_status_));
    success = false;
  }
  UpdateReadyCheckVolume(ready_check_volume);

  if (!UpdateSquadReady(squad_ready_path)) {
    logging::Squad(std::format("Failed to load squad ready audio from {}: {}",
                               squad_ready_path, squad_ready_status_));
    success = false;
  }
  UpdateSquadReadyVolume(squad_ready_volume);

  return success;
}

bool AudioPlayer::ReInit() {
  Destroy();
  return Init(ready_check_path_, ready_check_volume_, squad_ready_path_,
       squad_ready_volume_);
}

bool AudioPlayer::UpdateReadyCheck(const std::string& path) {
  if (path.empty()) {
    logging::Debug("loading default ready check");
    ready_check_sound_ =
        std::make_unique<WaveFile>(MAKEINTRESOURCE(READY_CHECK), engine_.get());
  } else {
    ready_check_sound_ = std::make_unique<WaveFile>(path, engine_.get());
  }
  const bool success = ready_check_sound_->IsValid();
  if (!success) {
    logging::Debug("failed to load ready check");
    ready_check_status_ = ready_check_sound_->ErrorMessage();
  } else {
    ready_check_status_ = "";
  }
  UpdateReadyCheckVolume(ready_check_volume_);
  return success;
}

bool AudioPlayer::UpdateSquadReady(const std::string& path) {
  if (path.empty()) {
    logging::Debug("loading default squad ready");
    squad_ready_sound_ =
        std::make_unique<WaveFile>(MAKEINTRESOURCE(SQUAD_READY), engine_.get());
  } else {
    squad_ready_sound_ = std::make_unique<WaveFile>(path, engine_.get());
  }
  const bool success = squad_ready_sound_->IsValid();
  if (!success) {
    logging::Debug("failed to load squad ready");
    squad_ready_status_ = squad_ready_sound_->ErrorMessage();
  } else {
    squad_ready_status_ = "";
  }
  UpdateSquadReadyVolume(squad_ready_volume_);
  return success;
}

void AudioPlayer::UpdateReadyCheckVolume(const int volume) {
  ready_check_volume_ = volume;
  if (!ready_check_sound_) return;
  ready_check_sound_->SetVolume(volume);
}

void AudioPlayer::UpdateSquadReadyVolume(const int volume) {
  squad_ready_volume_ = volume;
  if (!squad_ready_sound_) return;
  squad_ready_sound_->SetVolume(volume);
}

std::string AudioPlayer::ReadyCheckStatus() { return ready_check_status_; }

std::string AudioPlayer::SquadReadyStatus() { return squad_ready_status_; }

std::string AudioPlayer::OutputDeviceName() {
  return ma_engine_get_device(engine_.get())->playback.name;
}

void AudioPlayer::Destroy() {
  ready_check_sound_.reset();
  squad_ready_sound_.reset();
  if (engine_) {
    ma_engine_uninit(engine_.get());
  }
}

void AudioPlayer::PlayReadyCheck() const {
  logging::Debug("playing ready check");
  if (!engine_) return;
  if (!ready_check_sound_) return;
  ready_check_sound_->Play();
}

void AudioPlayer::PlaySquadReady() const {
  logging::Debug("playing squad ready");
  if (!engine_) return;
  if (!squad_ready_sound_) return;
  squad_ready_sound_->Play();
}

WaveFile::WaveFile() {
  valid_ = false;
  buffer_ = nullptr;
}

WaveFile::WaveFile(const std::string& file_name, ma_engine* engine) {
  valid_ = false;
  buffer_ = nullptr;
  sound_ = std::make_unique<ma_sound>();
  decoder_ = nullptr;

  const auto result =
      ma_sound_init_from_file(engine, file_name.c_str(), MA_SOUND_FLAG_DECODE,
                              nullptr, nullptr, sound_.get());
  if (result != MA_SUCCESS) {
    error_message_ = std::format("Failed to load: {}", error::humanize_ma_result(result));
    return;
  }

  valid_ = true;
}

WaveFile::WaveFile(LPWSTR resource, ma_engine* engine) {
  valid_ = false;
  buffer_ = nullptr;
  sound_ = std::make_unique<ma_sound>();
  decoder_ = std::make_unique<ma_decoder>();

  const auto resource_info = FindResource(globals::self_dll, resource, TEXT("WAVE"));
  if (resource_info == nullptr) return;

  const auto loaded_resource = LoadResource(globals::self_dll, resource_info);
  if (loaded_resource == nullptr) return;

  const auto resource_size = SizeofResource(globals::self_dll, resource_info);
  if (resource_size == 0) return;

  const auto resource_pointer = LockResource(loaded_resource);
  if (resource_pointer == nullptr) return;

  buffer_ = new char[resource_size];
  memcpy(buffer_, resource_pointer, resource_size);

  const auto decode_result =
      ma_decoder_init_memory(buffer_, resource_size, nullptr, decoder_.get());
  if (decode_result != MA_SUCCESS) {
    error_message_ = std::format("Internal error, failed to init decoder: {}", error::humanize_ma_result(decode_result));
    return;
  }

  const auto sound_init_result = ma_sound_init_from_data_source(
      engine, decoder_.get(), MA_SOUND_FLAG_DECODE, nullptr, sound_.get());
  if (sound_init_result != MA_SUCCESS) {
    error_message_ = std::format("Internal error, failed to init sound: {}",
                                 error::humanize_ma_result(sound_init_result));
    return;
  }

  valid_ = true;
}

WaveFile::~WaveFile() {
  logging::Debug("destroying wave file");
  valid_ = false;
  if (sound_) {
    ma_sound_stop(sound_.get());
    ma_sound_uninit(sound_.get());
  }
  if (decoder_) {
    ma_decoder_uninit(decoder_.get());
  }
  delete[] buffer_;
}

void WaveFile::Play() const {
  if (!valid_) {
    logging::Debug("wave file is not valid, not playing");
    return;
  }
  if (const auto engine = ma_sound_get_engine(sound_.get()); !engine) {
    return;
  }
  if (ma_sound_start(sound_.get()) != MA_SUCCESS) {
    logging::Debug("failed to play wave file");
  }
}

bool WaveFile::IsValid() const { return valid_; }

std::string WaveFile::ErrorMessage() const { return error_message_; }


void WaveFile::SetVolume(const int volume) const {
  if (!valid_) return;
  const float ratio = volume / 100.0f;
  ma_sound_set_volume(sound_.get(), ratio);
}
