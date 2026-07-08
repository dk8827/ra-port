#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ANDROID_DIR="$ROOT_DIR/android"
SDL_VERSION="2.32.10"
SDL_DIR="$ANDROID_DIR/third_party/SDL"
GRADLE_ARGS=()

usage() {
  cat <<USAGE
Usage: scripts/build_android_debug.sh [-- gradle-args...]

Fetch SDL2 Android sources if needed and run Gradle assembleDebug for the
local Android debug APK. Local ignored assets/redalert data is bundled only
into the debug APK.

Examples:
  scripts/build_android_debug.sh
  scripts/build_android_debug.sh -- --info

Output:
  android/app/build/outputs/apk/debug/app-debug.apk
USAGE
}

ensure_android_env() {
  if [[ -z "${JAVA_HOME:-}" && -d "/opt/homebrew/opt/openjdk@17/libexec/openjdk.jdk/Contents/Home" ]]; then
    export JAVA_HOME="/opt/homebrew/opt/openjdk@17/libexec/openjdk.jdk/Contents/Home"
    export PATH="$JAVA_HOME/bin:$PATH"
  fi
  if [[ -z "${ANDROID_HOME:-}" && -z "${ANDROID_SDK_ROOT:-}" && -d "$HOME/Library/Android/sdk" ]]; then
    export ANDROID_HOME="$HOME/Library/Android/sdk"
    export ANDROID_SDK_ROOT="$ANDROID_HOME"
  elif [[ -z "${ANDROID_HOME:-}" && -z "${ANDROID_SDK_ROOT:-}" && -d "/opt/homebrew/share/android-commandlinetools" ]]; then
    export ANDROID_HOME="/opt/homebrew/share/android-commandlinetools"
    export ANDROID_SDK_ROOT="$ANDROID_HOME"
  fi
}

verify_assets() {
  local config="$ROOT_DIR/assets/redalert/allies/INSTALL/REDALERT.INI"
  if [[ ! -f "$config" ]]; then
    echo "Missing local Red Alert assets under $ROOT_DIR/assets/redalert" >&2
    echo "Run scripts/prepare_assets_from_local.sh first." >&2
    exit 1
  fi
}

fetch_sdl2() {
  if [[ -f "$SDL_DIR/CMakeLists.txt" && -f "$SDL_DIR/android-project/app/src/main/java/org/libsdl/app/SDLActivity.java" ]]; then
    return
  fi

  local third_party="$ANDROID_DIR/third_party"
  local archive="$third_party/SDL2-$SDL_VERSION.tar.gz"
  local extracted="$third_party/SDL2-$SDL_VERSION"

  mkdir -p "$third_party"
  if [[ ! -f "$archive" ]]; then
    curl -L "https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VERSION/SDL2-$SDL_VERSION.tar.gz" -o "$archive"
  fi

  rm -rf "$extracted" "$SDL_DIR"
  tar -xzf "$archive" -C "$third_party"
  mv "$extracted" "$SDL_DIR"
}

run_gradle() {
  local gradle_cmd="gradle"
  if [[ -x "$ANDROID_DIR/gradlew" ]]; then
    gradle_cmd="$ANDROID_DIR/gradlew"
  elif ! command -v gradle >/dev/null 2>&1; then
    echo "Gradle is required. Install Gradle or add android/gradlew." >&2
    exit 1
  fi

  if [[ "${#GRADLE_ARGS[@]}" -gt 0 ]]; then
    "$gradle_cmd" -p "$ANDROID_DIR" :app:assembleDebug "${GRADLE_ARGS[@]}"
  else
    "$gradle_cmd" -p "$ANDROID_DIR" :app:assembleDebug
  fi
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      GRADLE_ARGS=("$@")
      break
      ;;
    *)
      GRADLE_ARGS+=("$1")
      shift
      ;;
  esac
done

ensure_android_env
verify_assets
fetch_sdl2
run_gradle

echo "Built $ANDROID_DIR/app/build/outputs/apk/debug/app-debug.apk"
