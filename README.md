# ra-port

**Native macOS and Android source port of Command & Conquer: Red Alert.**

[![macOS](https://img.shields.io/badge/macOS-native-111111?logo=apple&logoColor=white)](#quick-start)
[![Android](https://img.shields.io/badge/Android-debug%20APK-3ddc84?logo=android&logoColor=white)](#android-debug-apk)
[![Build](https://img.shields.io/badge/build-CMake%20%2B%20Ninja-064f8c)](#build-from-source)
[![Runtime](https://img.shields.io/badge/runtime-SDL2-cc3333)](#current-status)
[![Source-only](https://img.shields.io/badge/source--only-no%20game%20data-lightgrey)](#game-data)
[![License](https://img.shields.io/badge/license-GPLv3%20with%20additional%20terms-blue)](#license-and-notice)

`ra-port` lets you play Red Alert (1996) on modern platforms. It currently runs as a native macOS executable and as a local Android debug APK, with SDL2 providing the platform layer.

![Red Alert running natively in a macOS window](docs/images/ra-port-macos-window.png)

The repository contains only source code and build tooling. No game assets are included in the repository. To play, provide legally obtained Red Alert assets from your own discs, mounted images, or local backups.

## Why This Exists

Red Alert was released for a very different desktop world. This project keeps the original code recognizable while supporting macOS and Android.

This is an unofficial source port based on the source code Electronic Arts released under GPLv3 with additional terms: <https://github.com/electronicarts/CnC_Red_Alert>.

## Current Status

| Status | Feature | Notes |
| --- | --- | --- |
| :white_check_mark: | macOS on Apple Silicon | Builds and runs with CMake/Ninja. |
| :white_check_mark: | Android debug APK | Builds a local landscape APK for arm64-v8a devices and emulators. |
| :white_check_mark: | Campaign | Allied and Soviet campaigns are fully working. |
| :white_check_mark: | Skirmish | Local skirmish is fully working. |
| :white_check_mark: | Videos | Videos are playing with sound. |
| :white_check_mark: | Controls and audio | macOS keyboard/mouse and Android touch/audio work. |
| :x: | Online/network multiplayer | Not wired up yet. |
| :x: | Launcher/setup tools | Not ported. |
| :x: | Expansion packs | Not a focus yet. |
| :x: | `.app` bundle | Not packaged yet; the build creates a normal macOS executable. |
| :x: | Android release build | Only local debug APKs are supported right now. |

## Quick Start

Install the macOS build tools:

```sh
brew install cmake ninja pkg-config sdl2
xcode-select --install
```

Build the port:

```sh
cmake -S . -B build -G Ninja
cmake --build build --target redalert_mac -j 8
```

Prepare local game data:

```sh
scripts/prepare_assets_from_local.sh \
  --allies /path/to/allies-disc \
  --soviet /path/to/soviet-disc
```

Run:

```sh
scripts/run_mac_dev.sh --no-build
```

To build and run the Android debug APK, install the Android prerequisites listed below, keep the same prepared local game data under `assets/redalert`, then run:

```sh
scripts/build_android_debug.sh
scripts/run_android_debug.sh --no-build
```

## Game Data

The repository contains only source code and build tooling. It does not contain game data, movies, music, disc images, archives, installers, generated palettes, or packaged executables.

The asset preparation script copies from local paths that you provide:

- `assets/redalert/allies`
- `assets/redalert/soviet`

Those directories are ignored by git. They should contain original disc-root style files such as `INSTALL/REDALERT.INI` and the base-game `.MIX` files.

The Android debug build uses the same ignored `assets/redalert` tree. Gradle copies those local files into generated debug assets while building the APK; they are not checked in and they are not used for a release build.

## Build From Source

Configure and build:

```sh
cmake -S . -B build -G Ninja
cmake --build build --target redalert_mac -j 8
```

If you do not have Ninja installed, omit `-G Ninja` and CMake will choose the default generator.

The build currently creates a raw macOS executable:

```sh
build/redalert_mac
```

It is not packaged as a `.app` bundle yet.

## Run

The normal development run command builds if needed, verifies local assets, codesigns the executable, and launches from the repository root:

```sh
scripts/run_mac_dev.sh
```

Useful variants:

```sh
scripts/run_mac_dev.sh --no-build
scripts/run_mac_dev.sh --prepare-only
```

You can also run the built executable directly after codesigning:

```sh
codesign --force --sign - build/redalert_mac
./build/redalert_mac
```

Runtime files such as `SAVEGAME.*`, `OPTIONS.INI`, `ASSERT.TXT`, screenshots, logs, and generated palette caches are ignored by git.

## Android Debug APK

The Android target is for local development and testing only. It builds an arm64-v8a debug APK, locks the activity to landscape, uses touch-native input, and extracts the bundled debug assets into app storage on first launch.

Install Android tooling:

- JDK 17
- Gradle
- Android SDK Platform 36
- Android SDK Build Tools
- Android NDK `28.2.13676358`
- Android CMake package
- Android platform-tools for `adb`

Android Studio is the easiest way to install the SDK, NDK, CMake, emulator, and platform-tools. On Homebrew-based macOS setups, the helper scripts auto-detect common `openjdk@17` and Android SDK locations when `JAVA_HOME`, `ANDROID_HOME`, or `ANDROID_SDK_ROOT` are not already set.

Build the debug APK:

```sh
scripts/build_android_debug.sh
```

The first build downloads SDL2 sources into ignored local storage under `android/third_party/`. The APK is written to:

```text
android/app/build/outputs/apk/debug/app-debug.apk
```

Install and launch on a connected device or emulator:

```sh
scripts/run_android_debug.sh --no-build
```

If an emulator is low on space and in-place install fails, uninstall the old debug app first:

```sh
scripts/run_android_debug.sh --no-build --fresh-install
```

`--fresh-install` removes existing app data, including extracted assets and saves.

Build, install, launch, and tail Android logs:

```sh
scripts/run_android_debug.sh --logcat
```

Useful direct commands:

```sh
adb install -r android/app/build/outputs/apk/debug/app-debug.apk
adb shell am start -n com.raport.redalert/.RedAlertActivity
```

The debug APK intentionally includes your local ignored game data so the app can run on the device without external storage setup. Do not distribute that APK.

## Fullscreen

Start fullscreen:

```sh
RA_FULLSCREEN=1 scripts/run_mac_dev.sh
```

Toggle fullscreen while running:

```text
Command+Return
```

## Tests

Run the source-level tests and script checks:

```sh
tests/run_script_tests.sh
```

Validate a fresh checkout with a full build first:

```sh
cmake -S . -B build -G Ninja
cmake --build build --target redalert_mac -j 8
tests/run_script_tests.sh
```

## Project Layout

| Path | Purpose |
| --- | --- |
| `CODE/` | Main Red Alert game code |
| `PORT/MAC/` | macOS runtime, compatibility shims, SDL2 integration |
| `PORT/ANDROID/` | Android entrypoint and platform-specific resource setup |
| `android/` | Gradle Android app that builds the debug APK |
| `WIN32LIB/`, `WINVQ/` | Legacy support libraries used by the port |
| `scripts/` | Asset preparation, run helpers, smoke capture |
| `tests/` | Focused source-level and shim tests |
| `docs/images/` | README images only, not game data |

## Contributing

The port is intentionally conservative: keep original source layout and behavior recognizable, and prefer small platform-specific support files over broad rewrites. Good next areas are macOS packaging, Intel macOS validation, save/load hardening, expansion support, and eventually non-macOS platform layers.

Network and online multiplayer are out of scope for the current milestone.

## License And Notice

The source code is distributed under GPLv3 with additional terms. See `LICENSE.md`.

This is an unofficial modified source port. It is not affiliated with, endorsed by, sponsored by, or supported by Electronic Arts or any other rights holder. See `NOTICE.md`.
