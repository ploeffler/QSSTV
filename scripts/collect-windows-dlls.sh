#!/usr/bin/env bash
# Resolve transitive PE imports for the executable AND runtime-loaded plugins.
set -euo pipefail
stage="$(cd "$1" && pwd)"
prefix="$2"
system_dir="$3"
objdump_tool="${OBJDUMP:-objdump}"
declare -A seen
mapfile -d '' queue < <(find "$stage" -type f \( -iname '*.exe' -o -iname '*.dll' \) -print0)
while ((${#queue[@]})); do
  binary="${queue[0]}"; queue=("${queue[@]:1}")
  key="${binary,,}"
  [[ -z "${seen[$key]:-}" ]] || continue
  seen[$key]=1
  # Keep objdump failures visible instead of hiding them in process substitution.
  imports="$("$objdump_tool" -p "$binary")"
  while IFS= read -r dll; do
    [[ -n "$dll" ]] || continue
    case "${dll,,}" in api-ms-win-*.dll|ext-ms-win-*.dll) continue;; esac
    bundled="$(find "$stage" -maxdepth 1 -type f -iname "$dll" -print -quit)"
    if [[ -n "$bundled" ]]; then queue+=("$bundled");continue;fi
    source="$(find "$prefix/bin" -maxdepth 1 -type f -iname "$dll" -print -quit)"
    if [[ -n "$source" ]]; then
      cp "$source" "$stage/";queue+=("$stage/$(basename "$source")")
    elif [[ -z "$(find "$system_dir" -maxdepth 1 -type f -iname "$dll" -print -quit)" ]]; then
      echo "Missing runtime dependency $dll imported by $binary" >&2;exit 1
    fi
  done < <(sed -n 's/^[[:space:]]*DLL Name:[[:space:]]*//p' <<< "$imports" | tr -d '\r')
done
