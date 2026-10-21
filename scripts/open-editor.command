#!/bin/zsh
set -eu
project_dir="${0:A:h:h}"
engine_dir="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
python3 "$project_dir/scripts/prepare-eos-guest.py" --engine "$engine_dir"
exec "$engine_dir/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$project_dir/RespawnAndRage.uproject" "$@"
