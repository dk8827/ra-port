#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APK="$ROOT_DIR/android/app/build/outputs/apk/debug/app-debug.apk"
TAIL_LOGCAT=0
BUILD_ARGS=()

usage() {
  cat <<'USAGE'
Usage: scripts/run_android_debug.sh [options] [-- gradle-args...]

Build, install with adb, and launch the Android debug APK.

Options:
  --no-build        Reuse the existing APK
  --logcat          Tail adb logcat after launch
  -h, --help        Show this help
USAGE
}

DO_BUILD=1

while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-build)
      DO_BUILD=0
      shift
      ;;
    --logcat)
      TAIL_LOGCAT=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      BUILD_ARGS=("$@")
      break
      ;;
    *)
      BUILD_ARGS+=("$1")
      shift
      ;;
  esac
done

if ! command -v adb >/dev/null 2>&1; then
  echo "adb is required. Install Android platform-tools or add adb to PATH." >&2
  exit 1
fi

if [[ "$DO_BUILD" -eq 1 ]]; then
  if [[ "${#BUILD_ARGS[@]}" -gt 0 ]]; then
    "$ROOT_DIR/scripts/build_android_debug.sh" -- "${BUILD_ARGS[@]}"
  else
    "$ROOT_DIR/scripts/build_android_debug.sh"
  fi
fi

if [[ ! -f "$APK" ]]; then
  echo "Missing APK $APK" >&2
  exit 1
fi

adb install -r "$APK"
adb shell am start -n com.raport.redalert/.RedAlertActivity

if [[ "$TAIL_LOGCAT" -eq 1 ]]; then
  adb logcat -v time SDL:V RedAlertActivity:V '*:S'
fi
