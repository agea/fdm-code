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

- Set default selects Balanced, also used by fresh profiles and previously
  saved default choices. Custom saved preferences are preserved; selecting an
  individual option enters custom mode.
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

- Lumen quality (medium/high), post processing (AO/bloom scalability) and a
  frame rate limit (60/90/120/144/unlimited; the default preset uses 90).
- Decorative effects also bound local bullet holes (16/32/64/128), block impact
  effects (4/8/16/64) and turn cloth simulation off at minimum.

Presets (all without motion blur/DOF):

| Preset | Resolution | AA | GI | Refl. | Shadows | Effects | Post | Tex | Cap |
|---|---|---|---|---|---|---|---|---|---|
| Competitive | manual 67% | TAAU | off | off | off | minimum | low | medium | 144 |
| Performance | automatic | TAAU | off | off | low | low | medium | medium | 90 |
| Balanced | automatic | TSR | Lumen medium | SSR | medium | medium | high | high | 90 |
| Quality | automatic | TSR | Lumen high + HW RT | Lumen | high | high | ultra | ultra | 90 |

Competitive uses manual resolution because the automatic governor targets 60
FPS and would spend spare headroom on resolution. The manual slider retains 67%.

Validation: build-editor.command; native automation tests
EclipseRecoil.Menu.PerformancePreferences and
EclipseRecoil.Player.ResolutionHysteresis. The first uses an isolated temporary
INI, reloads it from disk when writes are enabled (use
-MultiprocessSaveConfig with -Multiprocess), and checks the preset's renderer CVars. Visual review
used the Metal game at 1280×900. No measured FPS improvement or cross-platform
GPU validation is implied.


30 ottobre2026: Competitive conserva la qualità High dei materiali
(`r.MaterialQualityLevel=1`), mantenendo effetti/ombre/cloth al minimo e lo stesso
budget texture. Le permutazioni Low dei materiali Paragon non vengono quindi
selezionate da questo preset. Default seleziona ora Balanced;
Performance mantiene Medium, Balanced/Quality High. È una misura conservativa
per la pelle arancione segnalata in Central Building: il difetto non è stato
riprodotto localmente, quindi la risoluzione sulla scena dell'utente resta
da confermare. Il costo GPU del percorso materiali High non è stato misurato.

Verifica successiva dell'utente con play.command: pelle normale in Competitive.
La causa precisa delle chiazze precedenti non è stata isolata nei render locali.
