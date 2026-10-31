#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP="$ROOT_DIR/ios/build-device/Debug-iphoneos/redalert_ios.app"
OUT="$ROOT_DIR/ios/build-device/RedAlert.ipa"

usage() {
  cat <<'USAGE'
Usage: scripts/package_ios_ipa.sh [--app PATH] [--output PATH]

Wrap a built iOS app bundle in an .ipa for sideloading tools such as SideStore
or AltStore, which re-sign it with your own Apple ID.

Build the app first with scripts/build_ios_debug.sh --device.

Options:
  --app PATH       App bundle (default: ios/build-device/Debug-iphoneos/redalert_ios.app)
  --output PATH    Output .ipa (default: ios/build-device/RedAlert.ipa)
  -h, --help       Show this help

The .ipa bundles your local game data. Keep it on your own devices; do not
share or upload it.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --app)
      [[ $# -ge 2 ]] || { echo "--app requires a path" >&2; exit 2; }
      APP="$2"
      shift 2
      ;;
    --output)
      [[ $# -ge 2 ]] || { echo "--output requires a path" >&2; exit 2; }
      OUT="$2"
      shift 2
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

[[ -d "$APP" ]] || { echo "App bundle not found: $APP" >&2; echo "Run scripts/build_ios_debug.sh --device first." >&2; exit 1; }

staging="$(mktemp -d)"
trap 'rm -rf "$staging"' EXIT
mkdir -p "$staging/Payload"
cp -R "$APP" "$staging/Payload/"
mkdir -p "$(dirname "$OUT")"
OUT="$(cd "$(dirname "$OUT")" && pwd -P)/$(basename "$OUT")"
rm -f "$OUT"
(cd "$staging" && zip -qry "$OUT" Payload)
echo "Packaged $OUT ($(du -h "$OUT" | cut -f1))"
