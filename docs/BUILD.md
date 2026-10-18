# Local macOS builds

Use these commands from the game repository. The Mac build uses the locally
installed Unreal Engine and Xcode directly. It needs no separate builder
repository, container, AWS account or CI runner.

## Prerequisites

- Unreal Engine **5.8.3**, normally at `/Users/Shared/Epic Games/UE_5.8`.
- Full Xcode selected as the active developer directory, with its license and
  first-launch setup completed. Xcode 27.1 was used for the first package.
- Python **3.10 or newer** and Git.
- Access to the three private asset repositories and Git LFS. Prepare and check
  the asset mounts before packaging:

```sh
git lfs install
python3 scripts/setup-assets.py --init
python3 scripts/setup-assets.py --check
xcodebuild -version
xcrun --find clang
```

The prepared app icon and Retina template are already versioned under
`Build/Mac/Resources`. Pillow is needed only when regenerating branding with
`python3 scripts/prepare-branding.py`, not for an ordinary build.

## Build the app

```sh
python3 scripts/package-macos.py --check
python3 scripts/package-macos.py
```

The default is a **Development universal** app containing Apple Silicon
(`arm64`) and Intel (`x86_64`) executables. The script checks the final binary
with `lipo`, verifies Retina metadata and requires the embedded application icon.
It creates an isolated project snapshot and follows the private Content mounts.
Compilation and cooking take place in that copy, preserving the live checkout.
Only the runtime background and icon are included from Branding; source artwork
under Branding/Originals is not shipped.

Paths are relative to the repository's parent, independent of the current
shell directory:

```text
../builds/macos/<timestamp>/
  dist/RespawnAndRage.app
  build.log
  receipt.json
  project/                   # isolated source/assets and generated build files
```

Open `dist/RespawnAndRage.app` in Finder after a successful build. The receipt
records the engine version, source commit, commands and verified architectures.
The output contains private licensed assets; keep it out of public source commits.

## Game version

Every native Game or Editor build stamps `YY.MM.DD.<8-character-commit>` using
the build machine's local calendar date and the source checkout's HEAD. The main
menu shows it at the bottom right over the background. For example:
`26.10.18.b3e95fe5`. Uncommitted edits do not change the hash.

Unreal's pre-build step runs `scripts/stamp-build-version.py` (Python 3 on PATH;
`python3` on macOS/Linux, `python` on Windows). It generates the ignored
`Build/GameVersion.txt`, staged as a loose runtime dependency on all platforms.
The app reads this packaged file, so its version stays fixed after installation
and requires no Git or Python at runtime. Builds that skip compilation also skip
the pre-build step and retain the existing stamp.

The Mac helper carries the original full commit into isolated snapshots in an
ignored `Build/SourceCommit.txt`; it also records the displayed version in the
receipt. Source exports without `.git` must supply this file with the original
full commit hash before building. A missing identity fails the stamping step.

## Options

```sh
# Apple Silicon only, with a smaller compilation footprint
python3 scripts/package-macos.py --architecture arm64

# Intel only
python3 scripts/package-macos.py --architecture x64

# Shipping configuration (the initial verified package was Development)
python3 scripts/package-macos.py --configuration Shipping

# Different engine installation or build disk
python3 scripts/package-macos.py \
  --engine '/path/to/UE_5.8' \
  --workspace '/Volumes/BuildSSD/RespawnAndRage/mac'
```

The workspace must be outside the game checkout. On APFS, the script probes
copy-on-write cloning: asset snapshots share storage initially while edits to
either copy remain independent. Other filesystems use normal copies.

The initial free-space guards are **25 GiB for universal**, **20 GiB for a
single architecture** with APFS clones, or **40 GiB for ordinary copies**. These
are working margins rather than guaranteed peaks. A watchdog stops the build's
own processes if less than **5 GiB** remains. The first universal package was
about **3.2 GiB**; retained intermediates, cooking data and caches use more space.

## Follow progress and resume

The build command remains running while child-tool output goes to `build.log`:

```sh
tail -f ../builds/macos/<timestamp>/build.log
```

To reuse a failed attempt, pass the same workspace, architecture and configuration:

```sh
python3 scripts/package-macos.py --resume ../builds/macos/<timestamp>
```

A resume keeps the existing snapshot, so later edits in the live checkout do not
enter that attempt. Start a fresh build to include newer source or asset edits.
An attempt whose generated files were cleaned cannot be resumed.

UE 5.8.3 on macOS 27 / Xcode 27 has a known closed-stdin failure during Xcode
app finalization. The script automatically runs `ApplePostBuildSync` outside UBA
with valid stdin on that combination, without patching the installed engine.
`--xcode-stdin-workaround` requests the same route explicitly on other versions.

## Reclaim space after success

Keep `dist/RespawnAndRage.app`, `build.log` and `receipt.json`. Once the build has
finished and the packaged app has been checked, the generated directories under
that attempt's `project/Intermediate`, `project/Saved` and `project/Binaries`
can be removed. These are inside the isolated build workspace, not the live
repository. Mark the receipt with `"scratch_removed": true` when doing so to
prevent a resume that relies on deleted files. Fresh builds regenerate them.

Preserve the live project's assets, source artwork, Git/LFS, settings and
autosaves. The global Unreal DDC/Zen caches speed up later cooking; they are not
part of the packaged app.

## Current validation and distribution

The first universal package passed signature verification and Metal offscreen
startup/map-load/clean-exit checks for Main Menu and Depot on Apple Silicon.
The corrected Retina package uses native backing pixels and the custom Finder
icon. Intel execution and a Shipping package have not yet been tested.

The generated app is locally signed; Developer ID signing and notarization for
public distribution are separate steps. No upload to itch.io is performed by
the local build command.
