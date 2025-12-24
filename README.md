# arcdps Squad Ready plugin

Plugin for arcdps to play audio files when a ready check has started and completed.

## Demo

https://user-images.githubusercontent.com/818368/169086436-0515382a-b121-43c7-acb8-727ec08a124d.mp4

## Installation

[arcdps](https://www.deltaconnected.com/arcdps/) and the [unofficial extras plugin](https://github.com/Krappa322/arcdps_unofficial_extras_releases) should be installed.

### Guild Wars 2 Unofficial Add-On Manager

The plugin can be installed via the [Guild Wars 2 Unofficial Add-On Manager](https://github.com/gw2-addon-loader/GW2-Addon-Manager).

### Manual

Download the [latest release from the Releases page](https://github.com/cheahjs/arcdps-squad-ready-plugin/releases/latest).

Place the .dll file into the same folder as arcdps.

## Usage

The plugin will play sounds when a ready check has started, and when the ready check has completed and the squad is ready.

By default, the sounds are English text-to-speech of the phrases "Ready check" and "Squad ready".

To setup custom sounds and volume levels, you can open the arcdps options panel (Alt+Shift+T by default) and adjust the volume levels for each event, and you can also provide a path to a MP3, WAV, or FLAC file to customize the sound.

The plugin can be set to "nag" with the ready check started sound on an interval if you are not readied up.

![screenshot of options](https://user-images.githubusercontent.com/818368/212587541-5edc2557-16ca-44ef-9b9f-05d493b63cac.png)

## Known Issues

* If you are already in a squad (a pending invite counts as well) when the game is started, this addon will not track existing players until an update occurs (such as moving subgroups, hitting the ready button). This means the ready check and squad ready sounds may or may not play at incorrect times.
* If there is a pending invite and a ready check occurs, the squad ready sound will not play.

## Building

### Prerequisites

- CMake 3.25 or later
- [vcpkg](https://github.com/microsoft/vcpkg) (set `VCPKG_ROOT` environment variable)
- Ninja build system (recommended)

### Building on Windows (Command Line)

Requires Visual Studio 2022 with C++ workload installed.

```bash
cmake --preset windows-x64-release
cmake --build --preset windows-x64-release
```

The built DLL will be in `build/windows-x64-release/squad_ready/arcdps_squad_ready.dll`.

### Using Visual Studio IDE

To develop using Visual Studio, generate a solution file:

```bash
cmake --preset vs2022
```

Then open `build/vs2022/arcdps-squad-ready-plugin.sln` in Visual Studio, or build from command line:

```bash
cmake --build --preset vs2022-release
```

### Cross-compiling from macOS/Linux

Install MinGW-w64:

```bash
# macOS (Homebrew)
brew install mingw-w64 ninja

# Linux (Ubuntu/Debian)
sudo apt install mingw-w64 ninja-build
```

Build:

```bash
cmake --preset cross-mingw-release
cmake --build --preset cross-mingw-release
```

The built DLL will be in `build/cross-mingw-release/squad_ready/arcdps_squad_ready.dll`.