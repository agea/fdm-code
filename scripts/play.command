#!/bin/zsh
set -eu
project_dir="${0:A:h:h}"
engine_dir="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
# Native startup restores saved display preferences; first launch is fullscreen.
# Explicit command-line overrides remain available for captures and local tests.
exec "$engine_dir/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$project_dir/RespawnAndRage.uproject" /Game/EclipseRecoil/Maps/L_MainMenu \
  -game -MULTIHOME=0.0.0.0 -NoSplash '-ExecCmds=t.MaxFPS 90,sg.AntiAliasingQuality 2,t.IdleWhenNotForeground 0,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.ShadowQuality 1' "$@"
