# Performance settings

Setup → Performance is available from the main menu and the in-game menu,
in a centered panel spanning the window with 70-unit side margins. Settings
use two equal columns when the content is at least 900 Slate units wide and
reflow to one column below that width, including during window resizing. Labels
and controls remain together. The preset and default buttons also sit side by
side on wide windows. The same form layout is used in Setup, Play Offline,
Start Server and Equipment; other submenus also use the centered panel.
Preferences are local, applied immediately and saved in the existing
GameUserSettings.ini under EclipseRecoil.Performance. Game/network state and
source assets are unaffected.

- Set default restores the renderer values captured before Performance overrides,
  original cosmetic debris behavior and automatic resolution. A fresh profile
  uses these original project defaults. Existing saved Performance choices are
  preserved; selecting an individual option enters custom mode.
- Automatic resolution preserves the existing 60 FPS governor and 1080p scene
  output cap. Manual resolution uses a 1–100% slider relative to the window;
  the UI remains at viewport resolution. Manual mode fixes the primary fraction,
  so the governor cannot undo the user's choice. TSR/TAAU retain a supported
  temporal input fraction; secondary spatial scaling handles extremely low
  requested scales. TAA uses spatial scaling directly.
- Antialiasing: reduced-history TSR, TAA with spatial scaling, or TAAU.
- Global illumination: Lumen or disabled. Hardware RT: disabled or requested
  when supported by the platform/project. This does not enable unsupported
  hardware or change the project's shader compilation support.
- Shadows: off/low/medium/high, including resolution, distance, virtual-shadow
  resolution bias and local-light projection budget.
- Reflections: off, screen-space, or Lumen. Independent motion blur and DOF
  controls set the renderer quality gates; enabled effects still depend on the
  camera/post-process volume's authored settings.
- Decorative effects: scalability plus local debris caps (0/8/32/64, also bounded
  by the map's own cap), shorter debris lifetime at lower quality, and no ground
  chips at minimum/low. New debris uses the selected level. Gameplay smoke,
  destruction, damage and replicated debris payloads are unchanged.
- Textures: four quality levels, with mip bias/anisotropy and streaming pool
  constrained to the hardware memory budget where supported. The UI does not
  claim a dedicated VRAM amount on unified-memory GPUs.

The Performance preset selects automatic resolution, TAAU, no Lumen, no hardware
RT, no reflections, no motion blur/DOF and low shadows/effects/medium textures.
The manual slider retains 67% for switching to manual mode.

Validation: build-editor.command; native automation tests
EclipseRecoil.Menu.PerformancePreferences and
EclipseRecoil.Player.ResolutionHysteresis. The first uses an isolated temporary
INI, reloads it from disk when writes are enabled (use
-MultiprocessSaveConfig with -Multiprocess), and checks the preset's renderer CVars. Visual review
used the Metal game at 1280×900. No measured FPS improvement or cross-platform
GPU validation is implied.
