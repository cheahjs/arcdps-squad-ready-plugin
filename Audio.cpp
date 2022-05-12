#include "Audio.h"

bool AudioPlayer::Init(std::string ready_check_path,
                         std::string squad_ready_path) {
  bool success = true;
  if (!UpdateReadyCheck(ready_check_path)) {
    logging::Squad(std::format("Failed to load ready check audio from {}",
                               ready_check_path));
    success = false;
  };
  if (!UpdateSquadReady(squad_ready_path)) {
    logging::Squad(std::format("Failed to load squad ready audio from {}",
                               squad_ready_path));
    success = false;
  }
  return success;
}

bool AudioPlayer::UpdateReadyCheck(std::string path) {
  bool success = false;
  if (path.empty()) {
    logging::Debug("loading default ready check");
    ready_check_sound = std::make_unique<WaveFile>(MAKEINTRESOURCE(READY_CHECK));
  } else {
    ready_check_sound = std::make_unique<WaveFile>(path);
  }
  success = ready_check_sound->IsValid();
  if (!success) {
    logging::Debug("failed to load ready check");
  }
  return success;
}

bool AudioPlayer::UpdateSquadReady(std::string path) {
  bool success = false;
  if (path.empty()) {
    logging::Debug("loading default squad ready");
    squad_ready_sound =
        std::make_unique<WaveFile>(MAKEINTRESOURCE(SQUAD_READY));
  } else {
    squad_ready_sound = std::make_unique<WaveFile>(path);
  }
  success = squad_ready_sound->IsValid();
  if (!success) {
    logging::Debug("failed to load squad ready");
  }
  return success;
}

void AudioPlayer::PlayReadyCheck() {
  logging::Debug("playing ready check");
  if (!ready_check_sound) return;
  ready_check_sound->Play();
}

void AudioPlayer::PlaySquadReady() {
  logging::Debug("playing squad ready");
  if (!squad_ready_sound) return;
  squad_ready_sound->Play();
}

WaveFile::WaveFile() {
  valid = false;
  buffer = nullptr;
}

WaveFile::WaveFile(std::string filename) {
  valid = false;
  buffer = nullptr;

  std::ifstream infile(filename, std::ios::binary);

  if (!infile) {
    logging::Squad(std::format("Failed to load file: {}", filename));
    return;
  }

  infile.seekg(0, std::ios::end);  // get length of file
  int length = infile.tellg();
  buffer = new char[length];       // allocate memory
  infile.seekg(0, std::ios::beg);  // position to start of file
  infile.read(buffer, length);     // read entire file

  infile.close();
  valid = true;
}

WaveFile::WaveFile(LPWSTR resource) {
  valid = false;
  buffer = nullptr;

  auto resource_info = FindResource(self_dll, resource, TEXT("WAVE"));
  if (resource_info == NULL) return;

  auto loaded_resource = LoadResource(self_dll, resource_info);
  if (loaded_resource == NULL) return;

  auto resource_size = SizeofResource(self_dll, resource_info);
  if (resource_size == 0) return;

  auto resource_pointer = LockResource(loaded_resource);
  if (resource_pointer == NULL) return;

  buffer = new char[resource_size];
  memcpy(buffer, resource_pointer, resource_size);

  valid = true;
}

WaveFile::~WaveFile() {
  logging::Debug("destroying wave file");
  PlaySound(NULL, 0, 0);  // Stop playing any sound before deleting buffer.
  delete[] buffer;
  valid = false;
}

void WaveFile::Play() {
  if (!valid) {
    logging::Debug("wave file is not valid, not playing");
    return;
  }
  PlaySoundA(buffer, NULL, SND_MEMORY | SND_ASYNC);
}

bool WaveFile::IsValid() { return valid; }
