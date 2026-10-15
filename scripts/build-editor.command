#!/bin/zsh
set -eu
project_dir="${0:A:h:h}"
engine_dir="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
exec "$engine_dir/Engine/Build/BatchFiles/Mac/Build.sh" \
  RespawnAndRageEditor Mac Development -Project="$project_dir/RespawnAndRage.uproject" \
  -architecture=arm64 -NoHotReloadFromIDE "$@"
