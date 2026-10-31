# Build

Daily compile and run. Packaging: [PACKAGE_MAC.md](PACKAGE_MAC.md), [PACKAGE_DESKTOP.md](PACKAGE_DESKTOP.md).

## Prerequisites

- Unreal Engine **5.8.3** (`/Users/Shared/Epic Games/UE_5.8` on Mac).
- Python 3.10+ on Mac (3.9+ on Windows, as `python.exe`), Git, Git LFS.
- Access to the three private asset repositories:

  ```sh
  git lfs install
  python3 scripts/setup-assets.py --init
  python3 scripts/setup-assets.py --check
  ```

- PCGExtendedToolkit 0.79.2 with source. Packaging copies enabled Marketplace code
  plugins into the build project as source-only and compiles them there.
- EOS guest play uses a generated, ignored plugin (`Plugins/OnlineSubsystemEOS`).
  Build and open helpers create it; before a manual IDE/UBT build run
  `python3 scripts/prepare-eos-guest.py --engine "/path/to/UE_5.8"`.

## Daily development (Mac)

```sh
scripts/build-editor.command   # compile the Editor module
scripts/open-editor.command    # open the project
scripts/play.command           # play in a separate window on the Editor runtime
```

Equivalent compile command:

```sh
"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" \
  RespawnAndRageEditor Mac Development -Project="$PWD/RespawnAndRage.uproject" \
  -architecture=arm64 -NoHotReloadFromIDE
```

Agents launching the editor from a shell: a recent project in `~/Downloads` can
block the home screen on a macOS privacy prompt until the user answers it.

**A Mac compile does not validate the other platforms.** Before pushing C++, follow
the cross-platform rules in [AGENTS.md](../AGENTS.md#cross-platform-c-rules).

## Channels and versions

Uploads go to `aaggeeaa/respawn-and-rage`: `mac-universal` (or `mac-arm64`, `mac-x86_64`),
`windows-x86_64`, `linux-x86_64`. Existing page access settings are retained. Butler
success does not prove the game starts: test launch, menus, maps and LAN on each OS.

Release builds require a clean Git tree, including untracked files and dirty submodules.

Each native build stamps `YY.MM.DD.<8-char commit>` through
`scripts/stamp-build-version.py` into the ignored `Build/GameVersion.txt`, shown at the
bottom right of the main menu. The date is the HEAD committer date. Isolated workspaces
and source exports without `.git` carry `Build/SourceCommit.txt` and
`Build/SourceCommitDate.txt`.
