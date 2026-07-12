#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: scripts/create_casefold_include_overlay.sh OUT_DIR ROOT...

Create a casefold symlink overlay for project include trees.

OUT_DIR is removed and recreated. Each ROOT is interpreted relative to the
current working directory unless it is absolute.
USAGE
}

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

if [[ $# -lt 2 ]]; then
  usage >&2
  exit 2
fi

OUT_DIR="$1"
shift

case "$OUT_DIR" in
  ""|"/"|".")
    fail "refusing unsafe output directory: $OUT_DIR"
    ;;
esac

absolute_path() {
  local path="$1"
  local dir
  dir="$(dirname "$path")"
  mkdir -p "$dir"
  dir="$(cd "$dir" && pwd -P)"
  printf '%s/%s\n' "$dir" "$(basename "$path")"
}

lower_path() {
  printf '%s' "$1" | tr '[:upper:]' '[:lower:]'
}

append_unique() {
  local value="$1"
  shift
  local existing
  for existing in "$@"; do
    [[ "$existing" == "$value" ]] && return 1
  done
  printf '%s\n' "$value"
}

path_variants() {
  local rel_path="$1"
  local old_ifs="$IFS"
  local parts
  local count
  local last
  local prefixes
  local dir_part
  local lower_dir_part
  local prefix
  local next_prefixes
  local base
  local lower_base
  local stem
  local ext
  local name
  local names

  IFS='/'
  read -r -a parts <<< "$rel_path"
  IFS="$old_ifs"

  count="${#parts[@]}"
  last=$((count - 1))
  prefixes=("")

  for ((index = 0; index < last; ++index)); do
    dir_part="${parts[$index]}"
    lower_dir_part="$(lower_path "$dir_part")"
    next_prefixes=()
    for prefix in "${prefixes[@]}"; do
      if [[ -n "$prefix" ]]; then
        next_prefixes+=("$prefix/$dir_part")
      else
        next_prefixes+=("$dir_part")
      fi
      if [[ "$lower_dir_part" != "$dir_part" ]]; then
        if [[ -n "$prefix" ]]; then
          next_prefixes+=("$prefix/$lower_dir_part")
        else
          next_prefixes+=("$lower_dir_part")
        fi
      fi
    done
    prefixes=("${next_prefixes[@]}")
  done

  base="${parts[$last]}"
  lower_base="$(lower_path "$base")"
  names=("$base")
  if append_unique "$lower_base" "${names[@]}" >/dev/null; then
    names+=("$lower_base")
  fi
  if [[ "$base" == *.* ]]; then
    stem="${base%.*}"
    ext=".${base##*.}"
    name="$(lower_path "$stem")$ext"
    if append_unique "$name" "${names[@]}" >/dev/null; then
      names+=("$name")
    fi
  fi

  for prefix in "${prefixes[@]}"; do
    for name in "${names[@]}"; do
      if [[ -n "$prefix" ]]; then
        printf '%s/%s\n' "$prefix" "$name"
      else
        printf '%s\n' "$name"
      fi
    done
  done
}

REPO_ROOT="$(pwd -P)"
OUT_DIR="$(absolute_path "$OUT_DIR")"
TMP_DIR="$OUT_DIR.tmp"

rm -rf "$TMP_DIR" "$OUT_DIR"
mkdir -p "$TMP_DIR"

link_overlay() {
  local source_path="$1"
  local overlay_rel_file="$2"
  local target="$TMP_DIR/$root_rel/$overlay_rel_file"
  local target_dir
  local existing

  target_dir="$(dirname "$target")"
  mkdir -p "$target_dir"

  if [[ -e "$target" || -L "$target" ]]; then
    existing="$(readlink "$target" || true)"
    if [[ "$existing" != "$source_path" ]]; then
      fail "casefold collision: $existing and $source_path both map to ${target#"$TMP_DIR/"}"
    fi
    return
  fi

  ln -s "$source_path" "$target"
}

for root in "$@"; do
  [[ -d "$root" ]] || fail "include root does not exist: $root"

  root_abs="$(cd "$root" && pwd -P)"
  case "$root_abs" in
    "$REPO_ROOT"/*)
      root_rel="${root_abs#"$REPO_ROOT"/}"
      root_rel="$(lower_path "$root_rel")"
      ;;
    *)
      fail "include root must be inside repo: $root"
      ;;
  esac

  index_file="$TMP_DIR/.casefold-index"
  includes_file="$TMP_DIR/.casefold-includes"
  : > "$index_file"
  : > "$includes_file"

  while IFS= read -r -d '' source_path; do
    rel_file="${source_path#"$root_abs"/}"
    printf '%s\t%s\n' "$(lower_path "$rel_file")" "$source_path" >> "$index_file"
    perl -ne 'print "$1\n" while /^\s*#\s*include\s*"([^"]+)"/g' "$source_path" >> "$includes_file"
    while IFS= read -r overlay_rel_file; do
      link_overlay "$source_path" "$overlay_rel_file"
    done < <(path_variants "$rel_file")
  done < <(find "$root_abs" -type f -print0)

  while IFS= read -r include_rel_file; do
    case "$include_rel_file" in
      ""|/*|../*|*"/../"*|*".."*)
        continue
        ;;
    esac
    include_lower="$(lower_path "$include_rel_file")"
    source_path="$(awk -F '\t' -v key="$include_lower" '$1 == key {print $2; exit}' "$index_file")"
    [[ -n "$source_path" ]] || continue
    link_overlay "$source_path" "$include_rel_file"
  done < <(sort -u "$includes_file")

  rm -f "$index_file" "$includes_file"
done

mv "$TMP_DIR" "$OUT_DIR"
