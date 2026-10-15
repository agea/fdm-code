#!/bin/zsh
set -eu
project_dir="${0:A:h:h}"
engine_dir="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
# Unreal high-DPI window sizes use pixels, while AppKit frames use points.
# Convert before launch so the first window (including the menu) is already 70%.
# Keep a usable fallback if launched without access to a graphical session.
window_width=1280
window_height=720
if window_size="$(/usr/bin/osascript -l JavaScript -e '
  ObjC.import("AppKit");
  const screen = $.NSScreen.mainScreen;
  if (!screen || !screen.frame) throw new Error("No screen available");
  const size = screen.frame.size;
  const scale = Number(screen.backingScaleFactor);
  [Math.round(size.width * scale * 0.7), Math.round(size.height * scale * 0.7)].join(" ");
' 2>/dev/null)" && [[ "$window_size" == <->' '<-> ]]; then
  read -r window_width window_height <<< "$window_size"
fi
exec "$engine_dir/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" \
  "$project_dir/fdm.uproject" /Game/EclipseRecoil/Maps/L_MainMenu \
  -game -MULTIHOME=0.0.0.0 -windowed "-ResX=$window_width" "-ResY=$window_height" -NoSplash '-ExecCmds=t.MaxFPS 90,r.VSync 0,sg.AntiAliasingQuality 2,t.IdleWhenNotForeground 0,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.ShadowQuality 1' "$@"
