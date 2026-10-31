#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IOS_DIR="$ROOT_DIR/ios"
SDL_VERSION="2.32.10"
SDL_DIR="$IOS_DIR/third_party/SDL"
CONFIGURATION="Debug"
PLATFORM="simulator"
CMAKE_ARGS=()

if [[ "$(uname -m)" == "arm64" ]]; then
  DEFAULT_SIM_ARCH="arm64"
else
  DEFAULT_SIM_ARCH="x86_64"
fi
ARCHS="$DEFAULT_SIM_ARCH"
SDK="iphonesimulator"
BUILD_DIR="$IOS_DIR/build-simulator"

usage() {
  cat <<'USAGE'
Usage: scripts/build_ios_debug.sh [options] [-- cmake-args...]

Fetch SDL2 iOS sources if needed and build the Red Alert iOS debug app with
CMake's Xcode generator.

Options:
  --simulator           Build for iphonesimulator (default, signing disabled)
  --device              Build for iphoneos arm64; requires RA_IOS_DEVELOPMENT_TEAM
  --archs ARCHS         CMAKE_OSX_ARCHITECTURES value (default: host simulator arch, or arm64 for device)
  --build-dir DIR       CMake build directory (default: ios/build-simulator or ios/build-device)
  -h, --help            Show this help

Environment:
  RA_IOS_DEVELOPMENT_TEAM   Apple development team id for --device signing

Examples:
  scripts/build_ios_debug.sh
  scripts/build_ios_debug.sh --device
  scripts/build_ios_debug.sh -- --trace-expand

Output:
  ios/build-simulator/Debug-iphonesimulator/redalert_ios.app
  ios/build-device/Debug-iphoneos/redalert_ios.app
USAGE
}

verify_assets() {
  local config="$ROOT_DIR/assets/redalert/allies/INSTALL/REDALERT.INI"
  if [[ ! -f "$config" ]]; then
    echo "Missing local Red Alert assets under $ROOT_DIR/assets/redalert" >&2
    echo "Run scripts/prepare_assets_from_local.sh first." >&2
    exit 1
  fi
}

ensure_xcode_developer_dir() {
  if [[ -n "${DEVELOPER_DIR:-}" ]]; then
    return
  fi
  if xcrun --sdk "$SDK" --show-sdk-path >/dev/null 2>&1; then
    return
  fi

  local developer_dir
  for developer_dir in \
    "/Applications/Xcode.app/Contents/Developer" \
    "/Applications/Xcode-beta.app/Contents/Developer"; do
    if [[ -d "$developer_dir/Platforms/iPhoneSimulator.platform" && -d "$developer_dir/Platforms/iPhoneOS.platform" ]]; then
      export DEVELOPER_DIR="$developer_dir"
      return
    fi
  done
}

verify_tools() {
  command -v cmake >/dev/null 2>&1 || { echo "cmake is required." >&2; exit 1; }
  command -v xcrun >/dev/null 2>&1 || { echo "Xcode xcrun is required." >&2; exit 1; }
  ensure_xcode_developer_dir
  xcrun --sdk "$SDK" --show-sdk-path >/dev/null
}

fetch_sdl2() {
  if [[ -f "$SDL_DIR/CMakeLists.txt" && -f "$SDL_DIR/src/main/uikit/SDL_uikit_main.c" ]]; then
    return
  fi

  local third_party="$IOS_DIR/third_party"
  local archive="$third_party/SDL2-$SDL_VERSION.tar.gz"
  local extracted="$third_party/SDL2-$SDL_VERSION"
  local android_archive="$ROOT_DIR/android/third_party/SDL2-$SDL_VERSION.tar.gz"

  mkdir -p "$third_party"
  if [[ ! -f "$archive" && -f "$android_archive" ]]; then
    cp "$android_archive" "$archive"
  fi
  if [[ ! -f "$archive" ]]; then
    curl -L "https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VERSION/SDL2-$SDL_VERSION.tar.gz" -o "$archive"
  fi

  rm -rf "$extracted" "$SDL_DIR"
  tar -xzf "$archive" -C "$third_party"
  mv "$extracted" "$SDL_DIR"
}

configure_project() {
  local cmake_args=(
    cmake -S "$IOS_DIR" -B "$BUILD_DIR" -G Xcode
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_SYSROOT="$SDK" \
    -DCMAKE_OSX_ARCHITECTURES="$ARCHS" \
    -DRA95_IOS_BUNDLED_ASSETS_DIR="$ROOT_DIR/assets/redalert"
  )
  if ((${#CMAKE_ARGS[@]})); then
    cmake_args+=("${CMAKE_ARGS[@]}")
  fi
  "${cmake_args[@]}"
}

build_project() {
  local xcode_settings=()
  if [[ "$PLATFORM" == "simulator" ]]; then
    xcode_settings+=(CODE_SIGNING_ALLOWED=NO)
  else
    if [[ -z "${RA_IOS_DEVELOPMENT_TEAM:-}" ]]; then
      echo "RA_IOS_DEVELOPMENT_TEAM is required for --device builds." >&2
      exit 1
    fi
    xcode_settings+=(DEVELOPMENT_TEAM="$RA_IOS_DEVELOPMENT_TEAM")
    xcode_settings+=(CODE_SIGNING_ALLOWED=YES)
    # Let Xcode create the development provisioning profile when none exists
    # yet, e.g. the first build for a free personal team.
    xcode_settings+=(-allowProvisioningUpdates)
  fi

  cmake --build "$BUILD_DIR" --config "$CONFIGURATION" --target redalert_ios -- "${xcode_settings[@]}"
}

print_app_path() {
  local app
  app="$(find "$BUILD_DIR" -type d -name redalert_ios.app -print -quit)"
  if [[ -n "$app" ]]; then
    echo "Built $app"
  else
    echo "Built redalert_ios, but could not locate redalert_ios.app under $BUILD_DIR" >&2
  fi
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --simulator)
      PLATFORM="simulator"
      SDK="iphonesimulator"
      ARCHS="$DEFAULT_SIM_ARCH"
      BUILD_DIR="$IOS_DIR/build-simulator"
      shift
      ;;
    --device)
      PLATFORM="device"
      SDK="iphoneos"
      ARCHS="arm64"
      BUILD_DIR="$IOS_DIR/build-device"
      shift
      ;;
    --archs)
      [[ $# -ge 2 ]] || { echo "--archs requires a value" >&2; exit 2; }
      ARCHS="$2"
      shift 2
      ;;
    --build-dir)
      [[ $# -ge 2 ]] || { echo "--build-dir requires a value" >&2; exit 2; }
      BUILD_DIR="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      CMAKE_ARGS=("$@")
      break
      ;;
    *)
      CMAKE_ARGS+=("$1")
      shift
      ;;
  esac
done

verify_assets
verify_tools
fetch_sdl2
configure_project
build_project
print_app_path
