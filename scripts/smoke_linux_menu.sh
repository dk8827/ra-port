#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build-linux"
SECONDS_TO_WAIT=16
SCREENSHOT=""
LOG_FILE=""
DO_BUILD=1
DO_ASSETS=1
KEEP_RUNNING=0

usage() {
  cat <<'USAGE'
Usage: scripts/smoke_linux_menu.sh [options]

Build and launch the Linux Red Alert port under Xvfb long enough to capture the title/menu.

Options:
  --seconds N            Seconds to wait before screenshot (default: 16)
  --screenshot PATH      Screenshot output path
  --log PATH             Log output path (default: build-linux/last_smoke_menu.log)
  --no-build             Reuse the existing build artifact
  --no-assets            Do not check local assets before running
  --build-dir DIR        CMake build directory (default: build-linux)
  --keep-running         Leave the game running after the screenshot
  -h, --help             Show this help
USAGE
}

absolute_dir() {
  mkdir -p "$1"
  (cd "$1" && pwd -P)
}

absolute_file() {
  local path="$1"
  local dir
  dir="$(dirname "$path")"
  mkdir -p "$dir"
  dir="$(cd "$dir" && pwd -P)"
  printf '%s/%s\n' "$dir" "$(basename "$path")"
}

require_command() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "Missing required command: $1" >&2
    exit 1
  }
}

start_xvfb_if_needed() {
  if [[ -n "${DISPLAY:-}" ]]; then
    return
  fi

  require_command Xvfb
  local display_number=$(( ($$ % 500) + 100 ))
  export DISPLAY=":$display_number"
  Xvfb "$DISPLAY" -screen 0 1024x768x24 > "$BUILD_DIR/xvfb.log" 2>&1 &
  XVFB_PID=$!
  sleep 1
}

stop_xvfb() {
  if [[ -n "${XVFB_PID:-}" ]]; then
    kill "$XVFB_PID" 2>/dev/null || true
    wait "$XVFB_PID" 2>/dev/null || true
  fi
}

assert_nonblank_screenshot() {
  require_command convert
  local mean
  mean="$(convert "$SCREENSHOT" -colorspace Gray -format '%[fx:mean]' info:)"
  awk -v mean="$mean" 'BEGIN { exit !(mean > 0.001) }' || {
    echo "Screenshot appears blank: $SCREENSHOT (mean=$mean)" >&2
    exit 1
  }
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --seconds)
      [[ $# -ge 2 ]] || { echo "--seconds requires a value" >&2; exit 2; }
      SECONDS_TO_WAIT="$2"
      shift 2
      ;;
    --screenshot)
      [[ $# -ge 2 ]] || { echo "--screenshot requires a value" >&2; exit 2; }
      SCREENSHOT="$2"
      shift 2
      ;;
    --log)
      [[ $# -ge 2 ]] || { echo "--log requires a value" >&2; exit 2; }
      LOG_FILE="$2"
      shift 2
      ;;
    --no-build)
      DO_BUILD=0
      shift
      ;;
    --no-assets)
      DO_ASSETS=0
      shift
      ;;
    --build-dir)
      [[ $# -ge 2 ]] || { echo "--build-dir requires a value" >&2; exit 2; }
      BUILD_DIR="$2"
      shift 2
      ;;
    --keep-running)
      KEEP_RUNNING=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

case "$SECONDS_TO_WAIT" in
  ''|*[!0-9]*)
    echo "--seconds must be a positive integer" >&2
    exit 2
    ;;
esac

if [[ "$SECONDS_TO_WAIT" -lt 1 ]]; then
  echo "--seconds must be a positive integer" >&2
  exit 2
fi

BUILD_DIR="$(absolute_dir "$BUILD_DIR")"
if [[ -z "$SCREENSHOT" ]]; then
  SCREENSHOT="$BUILD_DIR/redalert_menu_smoke.png"
fi
SCREENSHOT="$(absolute_file "$SCREENSHOT")"
if [[ -z "$LOG_FILE" ]]; then
  LOG_FILE="$BUILD_DIR/last_smoke_menu.log"
fi
LOG_FILE="$(absolute_file "$LOG_FILE")"

prepare_args=(--prepare-only --build-dir "$BUILD_DIR")
if [[ "$DO_BUILD" -eq 0 ]]; then
  prepare_args+=(--no-build)
fi
if [[ "$DO_ASSETS" -eq 0 ]]; then
  prepare_args+=(--no-assets)
fi

"$ROOT_DIR/scripts/run_linux_dev.sh" "${prepare_args[@]}"

require_command import
trap 'stop_xvfb' EXIT
start_xvfb_if_needed

pkill -TERM -x redalert_linux 2>/dev/null || true

cd "$ROOT_DIR"
SDL_VIDEODRIVER=x11 SDL_AUDIODRIVER=dummy "$BUILD_DIR/redalert_linux" > "$LOG_FILE" 2>&1 &
pid=$!

sleep "$SECONDS_TO_WAIT"

if ! kill -0 "$pid" 2>/dev/null; then
  wait "$pid"
  status=$?
  echo "redalert_linux exited before screenshot with status $status" >&2
  exit "$status"
fi

import -window root "$SCREENSHOT"
assert_nonblank_screenshot

echo "redalert_linux still running after ${SECONDS_TO_WAIT}s"
if [[ "$KEEP_RUNNING" -eq 0 ]]; then
  kill -TERM "$pid" 2>/dev/null || true
fi

echo "Screenshot: $SCREENSHOT"
echo "Log: $LOG_FILE"
