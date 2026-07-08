#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

assert_executable() {
  local path="$1"
  [[ -x "$ROOT_DIR/$path" ]] || fail "$path is not executable"
}

assert_help_contains() {
  local script="$1"
  local expected="$2"
  "$ROOT_DIR/$script" --help | grep -F -- "$expected" >/dev/null || fail "$script --help missing $expected"
}

assert_help_not_contains() {
  local script="$1"
  local unexpected="$2"
  if "$ROOT_DIR/$script" --help | grep -F -- "$unexpected" >/dev/null; then
    fail "$script --help should not contain $unexpected"
  fi
}

assert_file_contains() {
  local path="$1"
  local expected="$2"
  grep -F -- "$expected" "$ROOT_DIR/$path" >/dev/null || fail "$path missing $expected"
}

assert_file_not_contains() {
  local path="$1"
  local unexpected="$2"
  if grep -F -- "$unexpected" "$ROOT_DIR/$path" >/dev/null; then
    fail "$path should not contain $unexpected"
  fi
}

assert_executable scripts/run_mac_dev.sh
assert_executable scripts/prepare_assets_from_local.sh
assert_executable scripts/smoke_mac_menu.sh
assert_executable scripts/build_android_debug.sh
assert_executable scripts/run_android_debug.sh

legacy_stage_arg="--staging-""dir"
legacy_tmp_dir="/tmp/redalert_""mac_run"
legacy_stage_var="STAGING_""DIR"
legacy_stage_assets="stage_""assets"
legacy_stage_binary="stage_""binary"
legacy_staging_phrase="staging ""directory"
legacy_tmp_run_phrase="stage assets into a ""temporary run directory"

assert_help_contains scripts/run_mac_dev.sh "--prepare-only"
assert_help_contains scripts/run_mac_dev.sh "--no-build"
assert_help_contains scripts/run_mac_dev.sh "repo root"
assert_help_not_contains scripts/run_mac_dev.sh "$legacy_stage_arg"
assert_help_contains scripts/prepare_assets_from_local.sh "--allies"
assert_help_contains scripts/prepare_assets_from_local.sh "--soviet"
assert_help_contains scripts/smoke_mac_menu.sh "--seconds"
assert_help_contains scripts/smoke_mac_menu.sh "--screenshot"
assert_help_not_contains scripts/smoke_mac_menu.sh "$legacy_stage_arg"
assert_help_contains scripts/build_android_debug.sh "assembleDebug"
assert_help_contains scripts/run_android_debug.sh "adb"
assert_help_contains scripts/run_android_debug.sh "--fresh-install"

assert_file_not_contains scripts/run_mac_dev.sh "$legacy_tmp_dir"
assert_file_not_contains scripts/run_mac_dev.sh "$legacy_stage_var"
assert_file_not_contains scripts/run_mac_dev.sh "$legacy_stage_assets"
assert_file_not_contains scripts/run_mac_dev.sh "$legacy_stage_binary"
assert_file_not_contains scripts/run_mac_dev.sh 'rsync -a --delete "$source_dir/" "$target_dir/"'
assert_file_contains scripts/run_mac_dev.sh 'cd "$ROOT_DIR"'
assert_file_contains scripts/run_mac_dev.sh 'exec "$BUILD_DIR/$TARGET_NAME"'

assert_file_contains android/settings.gradle.kts "redalert-android"
assert_file_contains android/build.gradle.kts "com.android.application"
assert_file_contains android/app/build.gradle.kts "assembleDebug"
assert_file_contains android/app/build.gradle.kts "28.2.13676358"
assert_file_contains android/app/CMakeLists.txt "redalert_android"
assert_file_contains android/app/src/main/AndroidManifest.xml "landscape"
assert_file_contains android/app/src/main/AndroidManifest.xml "appCategory=\"game\""
assert_file_contains android/app/src/main/java/com/raport/redalert/RedAlertActivity.java "SDLActivity"
assert_file_contains android/app/src/main/java/com/raport/redalert/RedAlertActivity.java "extractBundledAssets"
assert_file_not_contains android/app/src/main/java/com/raport/redalert/RedAlertActivity.java "installOverlayControls"
assert_file_not_contains android/app/src/main/java/com/raport/redalert/RedAlertActivity.java "addKeyButton"
assert_file_not_contains android/app/src/main/java/com/raport/redalert/RedAlertActivity.java "addContentView(controls"
assert_file_not_contains android/app/src/main/java/com/raport/redalert/RedAlertActivity.java "LinearLayout.VERTICAL"
assert_file_not_contains android/app/src/main/java/com/raport/redalert/RedAlertActivity.java "Gravity.START | Gravity.CENTER_VERTICAL"
assert_file_contains android/app/src/main/java/com/raport/redalert/RedAlertActivity.java "KEYCODE_BACK"
assert_file_contains android/app/src/main/java/com/raport/redalert/RedAlertActivity.java "KEYCODE_ESCAPE"
assert_file_contains android/app/src/main/java/com/raport/redalert/RedAlertActivity.java "onBackPressed"
assert_file_contains android/app/src/main/java/com/raport/redalert/RedAlertActivity.java "OnBackInvokedDispatcher"
assert_file_contains android/app/src/main/java/com/raport/redalert/RedAlertActivity.java "registerOnBackInvokedCallback"
assert_file_contains android/app/src/main/java/com/raport/redalert/RedAlertActivity.java "WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE"
assert_file_contains android/app/src/main/java/com/raport/redalert/RedAlertActivity.java "WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars()"
assert_file_contains android/app/src/main/java/com/raport/redalert/RedAlertActivity.java "SYSTEM_UI_FLAG_IMMERSIVE_STICKY"
assert_file_contains PORT/ANDROID/src/android_main.cpp "SDL_main"
assert_file_contains PORT/MAC/src/mac_sdl_runtime.cpp "SDL_FINGERDOWN"
assert_file_contains PORT/MAC/src/mac_sdl_runtime.cpp "SDL_MULTIGESTURE"
assert_file_not_contains PORT/MAC/src/mac_sdl_runtime.cpp "SDL_CreateRenderer(MacWindow, -1, SDL_RENDERER_SOFTWARE);"
assert_file_contains PORT/MAC/src/mac_sdl_runtime.cpp "SDL_RENDERER_ACCELERATED"
assert_file_contains PORT/MAC/src/mac_sdl_runtime.cpp "mac_android_idle_delay"
assert_file_contains PORT/MAC/src/mac_sdl_runtime.cpp "SDL_Delay(1)"
assert_file_not_contains PORT/MAC/src/mac_sdl_runtime.cpp "RA_PERF"
assert_file_contains CODE/TAB.CPP "Android touch taps can land on the exact top row"
assert_file_contains WIN32LIB/KEYBOARD/MOUSE.CPP "mac_sdl_runtime.h"
assert_file_contains WIN32LIB/KEYBOARD/MOUSE.CPP "MacSDL_TouchCursorHidden()"

perl -0ne 'exit(/static void mac_queue_mouse_button_with_cursor\([^\)]*\)\s*\{\s*MacMousePoint\.x\s*=\s*x;\s*MacMousePoint\.y\s*=\s*y;[\s\S]*?if \(update_cursor\)/s ? 0 : 1)' "$ROOT_DIR/PORT/MAC/src/mac_sdl_runtime.cpp" \
  || fail "Android tap button events must update the legacy cursor position even when the touch cursor stays hidden"

perl -0ne 'exit(/void TabClass::AI\(KeyNumType &input, int x, int y\)[\s\S]*#if defined\(__ANDROID__\)[\s\S]*bool y_ok = y >= 0;[\s\S]*#else[\s\S]*bool y_ok = y > 0;[\s\S]*#endif/s ? 0 : 1)' "$ROOT_DIR/CODE/TAB.CPP" \
  || fail "Android top tab clicks must accept the exact top row without changing desktop edge behavior"

perl -0ne 'exit(/void WWMouseClass::Low_Show_Mouse\(int x, int y\)[\s\S]*MacSDL_TouchCursorHidden\(\)[\s\S]*MouseBuffX\s*=\s*-1[\s\S]*MouseBuffY\s*=\s*-1[\s\S]*return/s ? 0 : 1)' "$ROOT_DIR/WIN32LIB/KEYBOARD/MOUSE.CPP" \
  || fail "Android touch-hidden mode must suppress low-level software cursor drawing without leaving stale mouse backing coordinates"

perl -0ne 'exit(/void WWMouseClass::Draw_Mouse\(GraphicViewPortClass \*scr\)[\s\S]*MacSDL_TouchCursorHidden\(\)[\s\S]*return/s ? 0 : 1)' "$ROOT_DIR/WIN32LIB/KEYBOARD/MOUSE.CPP" \
  || fail "Android touch-hidden mode must suppress explicit software cursor draws"

for ignored_runtime_file in \
  "SAVEGAME.*" \
  "SAVEGAME.NET" \
  "OPTIONS.INI" \
  "ASSERT.TXT" \
  "*.PCX" \
  "*.pcx" \
  "*.LOG" \
  "*.log"; do
  assert_file_contains .gitignore "$ignored_runtime_file"
done

legacy_script="fetch_base_""assets.sh"
legacy_allies="RA_""Allies"
legacy_soviet="RA_""Soviet"
legacy_host="downloads."
legacy_host="${legacy_host}cnc-comm"

[[ ! -e "$ROOT_DIR/scripts/$legacy_script" ]] || fail "legacy asset downloader must not be exported"

for legacy_pattern in "$legacy_host" "$legacy_allies" "$legacy_soviet" "$legacy_script"; do
  grep -R -F "$legacy_pattern" "$ROOT_DIR/NOTICE.md" "$ROOT_DIR/scripts" >/dev/null \
    && fail "exported docs/scripts contain a legacy asset-download reference"
done

grep -F 'https://github.com/electronicarts/CnC_Red_Alert' "$ROOT_DIR/NOTICE.md" >/dev/null \
  || fail "NOTICE must reference the original EA source repository for provenance"

perl -0ne 'exit(/Buffer_Draw_Stamp_Clip\([^)]*\)\s*\{\s*\}/s ? 0 : 1)' "$ROOT_DIR/PORT/MAC/src/legacy_primitives.cpp" \
  && fail "Buffer_Draw_Stamp_Clip is still an empty stub"

perl -0ne 'exit(/Buffer_Frame_To_Page\([^)]*\)\s*\{\s*return Buffer_To_Page/s ? 0 : 1)' "$ROOT_DIR/PORT/MAC/src/legacy_primitives.cpp" \
  && fail "Buffer_Frame_To_Page must honor shape flags instead of raw copying"

perl -0ne 'exit(/apply_shape_effects[\s\S]*ghost_table\[256[\s\S]*Buffer_Frame_To_Page[\s\S]*va_start[\s\S]*SHAPE_GHOST/s ? 0 : 1)' "$ROOT_DIR/PORT/MAC/src/legacy_primitives.cpp" \
  || fail "Buffer_Frame_To_Page must apply SHAPE_GHOST remap tables"

perl -0ne 'exit(/Mouse_Shadow_Buffer\([^)]*\)\s*\{\s*\}/s ? 0 : 1)' "$ROOT_DIR/PORT/MAC/src/legacy_primitives.cpp" \
  && fail "Mouse_Shadow_Buffer is still an empty stub"

perl -0ne 'exit(/Draw_Mouse\([^)]*\)\s*\{\s*\}/s ? 0 : 1)' "$ROOT_DIR/PORT/MAC/src/legacy_primitives.cpp" \
  && fail "Draw_Mouse is still an empty stub"

perl -0ne 'exit(/ASM_Set_Mouse_Cursor\([^)]*\)\s*\{\s*return cursor;\s*\}/s ? 0 : 1)' "$ROOT_DIR/PORT/MAC/src/legacy_primitives.cpp" \
  && fail "ASM_Set_Mouse_Cursor must unpack cursor shapes"

perl -0ne 'exit(/VQA_Alloc\([^)]*\)\s*\{\s*return 0;\s*\}/s ? 0 : 1)' "$ROOT_DIR/PORT/MAC/src/legacy_primitives.cpp" \
  && fail "VQA_Alloc must return a handle so movie calls do not assert"

perl -0ne 'exit(/VQA_Open\([^)]*\)\s*\{\s*return VQAERR_OPEN;\s*\}/s ? 0 : 1)' "$ROOT_DIR/PORT/MAC/src/legacy_primitives.cpp" \
  && fail "VQA_Open must not force movie failure"

perl -0ne 'exit(/Is_Icon_Cached\([^)]*\)\s*\{\s*return 0;\s*\}/s ? 0 : 1)' "$ROOT_DIR/PORT/MAC/src/legacy_primitives.cpp" \
  && fail "Is_Icon_Cached must return -1 when the hardware cache is disabled"

grep -F 'dest_ptr + (y * 2) * dest_width' "$ROOT_DIR/PORT/MAC/src/legacy_primitives.cpp" >/dev/null \
  && fail "Asm_Interpolate must treat dest_width as the two-row stride passed by Interpolate_2X_Scale"

grep -F 'dst1 = dst0 + dest_width;' "$ROOT_DIR/PORT/MAC/src/legacy_primitives.cpp" >/dev/null \
  && fail "Asm_Interpolate must use half of dest_width for the second doubled row"

grep -F 'int row_pitch = dest_width / 2;' "$ROOT_DIR/PORT/MAC/src/legacy_primitives.cpp" >/dev/null \
  || fail "Asm_Interpolate must derive the single destination row pitch from dest_width"

grep -F 'char mac_cd_paths[] = "C:\\;D:\\";' "$ROOT_DIR/CODE/INIT.CPP" >/dev/null \
  && fail "bootstrap mixfile search must not force Allies before Soviet"

grep -F 'char mac_cd_paths[] = "?:\\";' "$ROOT_DIR/CODE/INIT.CPP" >/dev/null \
  || fail "bootstrap mixfile search must use the current-CD placeholder"

perl -0ne 'exit(/int Com_Scenario_Dialog\(bool\)\s*\{\s*return 0;\s*\}/s ? 0 : 1)' "$ROOT_DIR/PORT/MAC/src/legacy_primitives.cpp" \
  && fail "Com_Scenario_Dialog must not remain an unconditional stub"

perl -0ne 'exit(/int Com_Scenario_Dialog\(bool skirmish\)/s ? 0 : 1)' "$ROOT_DIR/PORT/MAC/src/legacy_primitives.cpp" \
  && fail "Com_Scenario_Dialog must use the original skirmish setup dialog instead of the mac shortcut"

perl -0ne 'exit(/list\(REMOVE_ITEM RA95_SOURCES[\s\S]*CODE\/NULLDLG\.CPP/s ? 0 : 1)' "$ROOT_DIR/CMakeLists.txt" \
  && fail "NULLDLG.CPP must be built so skirmish uses the original setup dialog"

perl -0ne 'exit(/Com_Scenario_Dialog\(bool skirmish\)[\s\S]*scenariolist[\s\S]*aiplayersgauge[\s\S]*difficulty/s ? 0 : 1)' "$ROOT_DIR/CODE/NULLDLG.CPP" \
  || fail "original skirmish setup dialog must expose scenario, AI-player, and difficulty controls"

grep -F 'SDL_WINDOW_FULLSCREEN_DESKTOP' "$ROOT_DIR/PORT/MAC/src/mac_sdl_runtime.cpp" >/dev/null \
  || fail "mac SDL runtime must support desktop fullscreen"

grep -F 'SDL_SetWindowFullscreen' "$ROOT_DIR/PORT/MAC/src/mac_sdl_runtime.cpp" >/dev/null \
  || fail "mac SDL runtime must be able to toggle fullscreen"

grep -F 'RA_FULLSCREEN' "$ROOT_DIR/PORT/MAC/src/mac_sdl_runtime.cpp" >/dev/null \
  || fail "mac SDL runtime must allow fullscreen from the run environment"

grep -F 'KMOD_GUI' "$ROOT_DIR/PORT/MAC/src/mac_sdl_runtime.cpp" >/dev/null \
  || fail "mac SDL runtime must support the mac Command key fullscreen shortcut"

perl -0ne 'exit(/game_idx = listbtn\.Current_Index\(\);[\s\S]{0,240}game_idx < 0[\s\S]{0,240}game_idx >= Files\.Count\(\)/s ? 0 : 1)' "$ROOT_DIR/CODE/LOADDLG.CPP" \
  || fail "load dialog must guard the selected save index before indexing Files"

grep -F 'last_scroll_tick != TickCount' "$ROOT_DIR/CODE/SCROLL.CPP" >/dev/null \
  || fail "edge scrolling must be paced to the game timer tick"

grep -F 'last_scroll_tick = TickCount' "$ROOT_DIR/CODE/SCROLL.CPP" >/dev/null \
  || fail "edge scrolling must record the timer tick it consumed"

grep -F 'int retval;' "$ROOT_DIR/CODE/MSGBOX.CPP" >/dev/null \
  || fail "message boxes must use an integer retval so the third button can return 2"

grep -F 'bool retval;' "$ROOT_DIR/CODE/MSGBOX.CPP" >/dev/null \
  && fail "message boxes must not coerce the third button return value through bool"

grep -F "if (Scen.Scenario == 1 && Scen.ScenarioName[2] != 'A')" "$ROOT_DIR/CODE/SCENARIO.CPP" >/dev/null \
  && fail "campaign scenario 1 must still require the side-specific base disc"

grep -F "Scen.ScenarioName[2] != 'U' && Scen.ScenarioName[2] != 'G'" "$ROOT_DIR/CODE/SCENARIO.CPP" >/dev/null \
  || fail "scenario 1 CD shortcut must not bypass Allied/Soviet campaign discs"

perl -0ne 'exit(/Session\.Type == GAME_SKIRMISH[\s\S]{0,120}Session\.Type = GAME_NORMAL[\s\S]{0,120}selection = SEL_NONE/s ? 0 : 1)' "$ROOT_DIR/CODE/INIT.CPP" \
  || fail "returning from skirmish must reset to the main menu instead of auto-starting skirmish"

grep -R -F "memcpy((char*)&Path" "$ROOT_DIR/CODE" >/dev/null \
  && fail "Path shifts must copy whole FacingType elements"

perl -0ne 'exit(/void Move_Point\([^)]*\)\s*\{(?:(?!void Normal_Move_Point)[\s\S])*static signed char const CosTable\[256\](?:(?!void Normal_Move_Point)[\s\S])*static signed char const SinTable\[256\]/s ? 0 : 1)' "$ROOT_DIR/CODE/COORD.CPP" \
  || fail "Move_Point must use signed trig tables so coordinate movement is portable across Android targets"

tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT
"${CXX:-c++}" -std=gnu++98 -I"$ROOT_DIR/PORT/MAC/include" "$ROOT_DIR/tests/ddraw_shim_test.cpp" -o "$tmpdir/ddraw_shim_test"
"$tmpdir/ddraw_shim_test"

"${CXX:-c++}" -std=gnu++98 -DTRUE_FALSE_DEFINED -I"$ROOT_DIR/CODE" \
  "$ROOT_DIR/tests/coord_cell_test.cpp" -o "$tmpdir/coord_cell_test"
"$tmpdir/coord_cell_test"

"${CXX:-c++}" -std=gnu++98 -DTRUE_FALSE_DEFINED -DBIG_ENDIAN=4321 -DLITTLE_ENDIAN=1234 -I"$ROOT_DIR/CODE" \
  "$ROOT_DIR/tests/fixed_endian_test.cpp" "$ROOT_DIR/CODE/FIXED.CPP" -o "$tmpdir/fixed_endian_test"
"$tmpdir/fixed_endian_test"

"${CXX:-c++}" -std=gnu++98 -I"$ROOT_DIR/PORT/MAC/include" \
  "$ROOT_DIR/tests/aspect_viewport_test.cpp" -o "$tmpdir/aspect_viewport_test"
"$tmpdir/aspect_viewport_test"

"${CXX:-c++}" -std=gnu++98 -I"$ROOT_DIR/PORT/MAC/include" \
  "$ROOT_DIR/tests/android_touch_gesture_test.cpp" -o "$tmpdir/android_touch_gesture_test"
"$tmpdir/android_touch_gesture_test"

"${CXX:-c++}" -std=gnu++98 -DTRUE_FALSE_DEFINED -I"$ROOT_DIR/PORT/MAC/include" -I"$ROOT_DIR/WIN32LIB/SHAPE" -I"$ROOT_DIR/WIN32LIB/INCLUDE" \
  "$ROOT_DIR/tests/shape_extract_test.cpp" "$ROOT_DIR/WIN32LIB/SHAPE/GETSHAPE.CPP" -o "$tmpdir/shape_extract_test"
"$tmpdir/shape_extract_test"

"${CXX:-c++}" -std=gnu++98 -DTRUE_FALSE_DEFINED -I"$ROOT_DIR/PORT/MAC/include" -I"$ROOT_DIR/WIN32LIB/INCLUDE" \
  "$ROOT_DIR/tests/cps_uncompress_test.cpp" "$ROOT_DIR/WIN32LIB/IFF/LOAD.CPP" -o "$tmpdir/cps_uncompress_test"
"$tmpdir/cps_uncompress_test"

"${CXX:-c++}" -std=gnu++98 -I"$ROOT_DIR/PORT/MAC/include" "$ROOT_DIR/tests/dos_compat_test.cpp" "$ROOT_DIR/PORT/MAC/src/dos_compat.cpp" -o "$tmpdir/dos_compat_test"
"$tmpdir/dos_compat_test"

"${CXX:-c++}" -std=gnu++98 -DTRUE_FALSE_DEFINED -I"$ROOT_DIR/PORT/MAC/include" -I"$ROOT_DIR/WIN32LIB/INCLUDE" -I"$ROOT_DIR/WIN32LIB/AUDIO" \
  "$ROOT_DIR/tests/audio_shim_test.cpp" "$ROOT_DIR/PORT/MAC/src/mac_audio_stub.cpp" $(pkg-config --cflags --libs sdl2) -o "$tmpdir/audio_shim_test"
"$tmpdir/audio_shim_test"

"${CXX:-c++}" -std=gnu++98 -I"$ROOT_DIR/PORT/MAC/include" "$ROOT_DIR/tests/timer_shim_test.cpp" "$ROOT_DIR/PORT/MAC/src/mac_timer.cpp" -o "$tmpdir/timer_shim_test"
"$tmpdir/timer_shim_test"

"${CXX:-c++}" -std=gnu++98 -I"$ROOT_DIR/PORT/MAC/include" "$ROOT_DIR/tests/input_shim_test.cpp" "$ROOT_DIR/PORT/MAC/src/mac_sdl_runtime.cpp" "$ROOT_DIR/PORT/MAC/src/mac_timer.cpp" $(pkg-config --cflags --libs sdl2) -o "$tmpdir/input_shim_test"
"$tmpdir/input_shim_test"

"${CXX:-c++}" -std=gnu++98 -DTRUE_FALSE_DEFINED -DVQADIRECT_SOUND=1 -I"$ROOT_DIR/PORT/MAC/include" -I"$ROOT_DIR/WINVQ/INCLUDE" -I"$ROOT_DIR/WINVQ/INCLUDE/VQM32" -I"$ROOT_DIR/WINVQ/INCLUDE/WWLIB32" \
  "$ROOT_DIR/tests/vqa_decode_test.cpp" "$ROOT_DIR/PORT/MAC/src/mac_vqa.cpp" "$ROOT_DIR/PORT/MAC/src/mac_timer.cpp" "$ROOT_DIR/CODE/LCWUNCMP.CPP" $(pkg-config --cflags --libs sdl2) -o "$tmpdir/vqa_decode_test"
"$tmpdir/vqa_decode_test"

"${CXX:-c++}" -std=gnu++98 -DTRUE_FALSE_DEFINED -DVQADIRECT_SOUND=1 -I"$ROOT_DIR/PORT/MAC/include" -I"$ROOT_DIR/WINVQ/INCLUDE" -I"$ROOT_DIR/WINVQ/INCLUDE/VQM32" -I"$ROOT_DIR/WINVQ/INCLUDE/WWLIB32" \
  "$ROOT_DIR/tests/vqa_file_smoke_test.cpp" "$ROOT_DIR/PORT/MAC/src/mac_vqa.cpp" "$ROOT_DIR/PORT/MAC/src/mac_timer.cpp" "$ROOT_DIR/CODE/LCWUNCMP.CPP" $(pkg-config --cflags --libs sdl2) -o "$tmpdir/vqa_file_smoke_test"
(cd "$ROOT_DIR" && SDL_AUDIODRIVER=dummy "$tmpdir/vqa_file_smoke_test")

echo "run script tests passed"
