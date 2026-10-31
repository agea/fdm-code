# Respawn & Rage

**Serious guns. Questionable players**

A tactical multiplayer FPS for quick team deathmatch with friends: compact seeded maps, fully
destructible 5cm-cell cover and server-authoritative gunplay. Built with Unreal Engine **5.8.3**
for macOS, Windows and Linux, playable offline, over LAN and over the Internet without an account.

- Five production maps: Central Building, Middle Eastern Alleys, Fire Training, Rooms, Village,
  each generated from a seed and sized to the expected team size.
- Ten-slot arsenal with CS2-derived ballistics, recoil and accuracy, plus Claymores and team smoke.
- Five roles with three variants each and 20 skins, ten per team.
- Team minimap with fog of war and reconnaissance drones.
- Configurable server rules, named presets and optional bots.

## Quick start

Code is public; licensed content lives in three private submodules, so a public clone needs asset
access before it can run.

```sh
git clone https://github.com/respawn-and-rage/core.git respawn-and-rage/core
cd respawn-and-rage/core
git lfs install
python3 scripts/setup-assets.py --init
scripts/build-editor.command
scripts/play.command
```

`scripts/open-editor.command` opens the project in Unreal.

## Controls

WASD move, mouse aim, left click fire, right click scope, Space jump/climb/respawn, Shift walk,
double-tap W or hold E sprint, Ctrl/C crouch, double-tap Ctrl/C prone, R reload, 1–8/F/G/H or
wheel select, Tab scoreboard, Esc menu.

## Documentation

| Read | For |
| --- | --- |
| [AGENTS.md](AGENTS.md) | Rules, invariants and which doc to read for each task |
| [docs/BUILD.md](docs/BUILD.md) | Setup, compile, versions |
| [docs/GAMEPLAY.md](docs/GAMEPLAY.md) | Rules, movement, server settings |
| [docs/WEAPONS.md](docs/WEAPONS.md) | Arsenal, damage, adding a weapon |
| [docs/MAPS.md](docs/MAPS.md) | Map generation; one page per map in `docs/maps/` |

The other guides are in [docs/](docs/).

Each build shows `YY.MM.DD.<commit>` at the bottom right of the main menu. Internal names such as
`fdm` and `/Game/EclipseRecoil` are kept for serialized compatibility.
