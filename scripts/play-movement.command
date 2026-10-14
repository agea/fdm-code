#!/bin/zsh
set -eu
project_dir="${0:A:h:h}"
engine_dir="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
exec "$engine_dir/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$project_dir/fdm.uproject" "/Game/EclipseRecoil/Maps/L_MovementLab?listen" \
  -game -windowed -ResX=1280 -ResY=720 -NoSplash '-ExecCmds=t.MaxFPS 90,r.VSync 0,sg.AntiAliasingQuality 2,t.IdleWhenNotForeground 0,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.ShadowQuality 1' "$@"
