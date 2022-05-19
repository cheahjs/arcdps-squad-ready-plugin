#include "Audio.h"

AudioPlayer::~AudioPlayer() {
  ready_check_sound.reset();
  squad_ready_sound.reset();
  if (engine) {
    ma_engine_uninit(engine.get());
  }
}

bool AudioPlayer::Init(std::string ready_check_path, int ready_check_volume,
                       std::string squad_ready_path, int squad_ready_volume) {
  bool success = true;

  engine = std::make_unique<ma_engine>();
  if (ma_engine_init(nullptr, engine.get()) != MA_SUCCESS) {
    logging::Squad("Failed to initialize audio engine");
    return false;
  }

  if (!UpdateReadyCheck(ready_check_path)) {
    logging::Squad(std::format("Failed to load ready check audio from {}",
                               ready_check_path));
    success = false;
  }
  UpdateReadyCheckVolume(ready_check_volume);

  if (!UpdateSquadReady(squad_ready_path)) {
    logging::Squad(std::format("Failed to load squad ready audio from {}",
                               squad_ready_path));
    success = false;
  }
  UpdateSquadReadyVolume(squad_ready_volume);

  return success;
}

bool AudioPlayer::UpdateReadyCheck(std::string path) {
  bool success = false;
  if (path.empty()) {
    logging::Debug("loading default ready check");
    ready_check_sound =
        std::make_unique<WaveFile>(MAKEINTRESOURCE(READY_CHECK), engine.get());
  } else {
    ready_check_sound = std::make_unique<WaveFile>(path, engine.get());
  }
  success = ready_check_sound->IsValid();
  if (!success) {
    logging::Debug("failed to load ready check");
    ready_check_status = "Failed to load";
  } else {
    ready_check_status = "";
  }
  UpdateReadyCheckVolume(ready_check_volume);
  return success;
}

bool AudioPlayer::UpdateSquadReady(std::string path) {
  bool success = false;
  if (path.empty()) {
    logging::Debug("loading default squad ready");
    squad_ready_sound =
        std::make_unique<WaveFile>(MAKEINTRESOURCE(SQUAD_READY), engine.get());
  } else {
    squad_ready_sound = std::make_unique<WaveFile>(path, engine.get());
  }
  success = squad_ready_sound->IsValid();
  if (!success) {
    logging::Debug("failed to load squad ready");
    squad_ready_status = "Failed to load";
  } else {
    squad_ready_status = "";
  }
  UpdateSquadReadyVolume(squad_ready_volume);
  return success;
}

void AudioPlayer::UpdateReadyCheckVolume(int volume) {
  ready_check_volume = volume;
  if (!ready_check_sound) return;
  ready_check_sound->SetVolume(volume);
}

void AudioPlayer::UpdateSquadReadyVolume(int volume) {
  squad_ready_volume = volume;
  if (!squad_ready_sound) return;
  squad_ready_sound->SetVolume(volume);
}

std::string AudioPlayer::ReadyCheckStatus() { return ready_check_status; }

std::string AudioPlayer::SquadReadyStatus() { return squad_ready_status; }

void AudioPlayer::PlayReadyCheck() {
  logging::Debug("playing ready check");
  if (!engine) return;
  if (!ready_check_sound) return;
  ready_check_sound->Play();
}

void AudioPlayer::PlaySquadReady() {
  logging::Debug("playing squad ready");
  if (!engine) return;
  if (!squad_ready_sound) return;
  squad_ready_sound->Play();
}

WaveFile::WaveFile() {
  valid = false;
  buffer = nullptr;
}

WaveFile::WaveFile(std::string filename, ma_engine* engine) {
  valid = false;
  buffer = nullptr;
  sound = std::make_unique<ma_sound>();
  decoder = nullptr;

  if (ma_sound_init_from_file(engine, filename.c_str(), MA_SOUND_FLAG_DECODE,
                              nullptr, nullptr, sound.get()) != MA_SUCCESS) {
    return;
  }

  valid = true;
}

WaveFile::WaveFile(LPWSTR resource, ma_engine* engine) {
  valid = false;
  buffer = nullptr;
  sound = std::make_unique<ma_sound>();
  decoder = std::make_unique<ma_decoder>();

  auto resource_info = FindResource(globals::self_dll, resource, TEXT("WAVE"));
  if (resource_info == NULL) return;

  auto loaded_resource = LoadResource(globals::self_dll, resource_info);
  if (loaded_resource == NULL) return;

  auto resource_size = SizeofResource(globals::self_dll, resource_info);
  if (resource_size == 0) return;

  auto resource_pointer = LockResource(loaded_resource);
  if (resource_pointer == NULL) return;

  buffer = new char[resource_size];
  memcpy(buffer, resource_pointer, resource_size);

  if (ma_decoder_init_memory(buffer, resource_size, NULL, decoder.get()) !=
      MA_SUCCESS) {
    return;
  }

  if (ma_sound_init_from_data_source(engine, decoder.get(), MA_SOUND_FLAG_DECODE,
                                     NULL, sound.get()) != MA_SUCCESS) {
    return;
  }

  valid = true;
}

WaveFile::~WaveFile() {
  logging::Debug("destroying wave file");
  valid = false;
  if (sound) {
    ma_sound_stop(sound.get());
    ma_sound_uninit(sound.get());
  }
  if (decoder) {
    ma_decoder_uninit(decoder.get());
  }
  delete[] buffer;
}

void WaveFile::Play() {
  if (!valid) {
    logging::Debug("wave file is not valid, not playing");
    return;
  }
  auto engine = ma_sound_get_engine(sound.get());
  if (!engine) {
    return;
  }
  if (ma_sound_start(sound.get()) != MA_SUCCESS) {
    logging::Debug("failed to play wave file");
    return;
  };
}

bool WaveFile::IsValid() { return valid; }

void WaveFile::SetVolume(int volume) {
  if (!valid) return;
  float ratio = volume / 100.0f;
  ma_sound_set_volume(sound.get(), ratio);
}
