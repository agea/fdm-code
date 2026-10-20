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
An attempt marked `scratch_removed=true` after full cleanup cannot be resumed.

To limit peak disk use, the helper removes completed Editor object/PCH files
after successful linking, retaining the Editor binaries and generated headers
needed for cooking. On the separate ApplePostBuildSync route it also removes
Game object/PCH files after finalization, before the -skipbuild cook. These
changes affect only the isolated snapshot; removed byte counts enter its receipt.

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

## Developer ID distribution and notarization

The 19 October request authorizes Mac/Linux builds and distribution on the
existing itch.io page. For Mac, install a **Developer ID Application** certificate
and its private key in the login Keychain (Xcode Settings → Accounts → Manage
Certificates). An Apple Developer account alone does not install an identity.
Confirm it with `security find-identity -v -p codesigning`.

Create a notarytool Keychain profile locally; enter credentials only in its
interactive prompts, never in source files or chat:

```sh
xcrun notarytool store-credentials RespawnAndRage
```

Use the Apple ID associated with the developer team, an app-specific password
and that team's ID. A supported App Store Connect API key is an alternative.
Run the helper against the final isolated build, using the exact identity name:

```sh
python3 scripts/notarize-macos.py /absolute/path/to/build/dist/RespawnAndRage.app \
  --identity 'Developer ID Application: Your Name (TEAMID)' \
  --keychain-profile RespawnAndRage --check

# Signs the app in place, submits to Apple, staples and checks Gatekeeper:
python3 scripts/notarize-macos.py /absolute/path/to/build/dist/RespawnAndRage.app \
  --identity 'Developer ID Application: Your Name (TEAMID)' \
  --keychain-profile RespawnAndRage
```

Nested Mach-O files and bundles are signed from the inside out, with a secure
timestamp and hardened runtime. Debugger entitlements are removed. The helper
requires Apple's Accepted status, validates the stapled ticket, checks the final
signature and runs `spctl --assess`. Receipt and rejection diagnostics live next
to `dist`, keeping the upload directory limited to the app. If hardened runtime
exposes an engine/library requirement, diagnose it with the packaged startup
test before granting any additional entitlement.

After success, run a packaged Metal startup/gameplay check, then upload the
unchanged app using `unreal-builder/scripts/publish-mac-itch.py`. Do not modify
the signed bundle after notarization. A notarized app can still display macOS's
ordinary first-open confirmation; the expected experience does not require an
override in Privacy & Security. Verify a browser-downloaded copy on another Mac.

Apple references: [Developer ID](https://developer.apple.com/developer-id/),
[notarization](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution),
[common issues](https://developer.apple.com/documentation/security/resolving-common-notarization-issues).

Local Developer ID creation requires the team's Account Holder. An Admin account
may instead use Xcode Organizer cloud signing if it has the dedicated cloud-managed
Developer ID permission. Apple Distribution is for App Store distribution and is
not a replacement for Developer ID when delivering directly through itch.io.
See [Developer ID roles](https://developer.apple.com/help/account/certificates/create-developer-id-certificates/).

With an authenticated Xcode account, `--sign-only` is an alternative first step
before Organizer's Developer ID → Upload workflow. Its receipt deliberately says
`signed-not-yet-notarized`. Export the notarized app from Organizer, then validate
its stapled ticket and Gatekeeper assessment before uploading it. A local
Developer ID signature alone does not satisfy the distribution requirement.
If xcodebuild reports `No Accounts` / missing `Xcode-Token`, refresh the account
through Xcode's UI; never extract its private authentication material.

## Linux snapshot with Marketplace dependencies

The cached Fargate Linux engine does not contain plugins installed in the local
Mac engine through Fab. Create the private input using the game helper:

```sh
python3 scripts/archive-linux-project.py /private/tmp/rr-linux-project.tar.gz --check
python3 scripts/archive-linux-project.py /private/tmp/rr-linux-project.tar.gz
```

Enabled Marketplace dependencies (currently PCGExtendedToolkit0.79.2) are included
as source-only project plugins. Binaries and Intermediate are excluded, and the
disposable plugin descriptor has Installed=false so Linux compiles its own code.
The installed Mac engine is preserved. The snapshot records plugin versions and
descriptor hashes in Build/ExternalPlugins.json. Do not commit the input archive
or acquired plugin sources to this public repository.

Upload this input privately, then run the existing Linux itch publisher task using
unreal-builder/scripts/run-task.py with --project-key and --publish-itch. Compilation,
cook, archive creation and channel upload run entirely on Fargate. The local
source archive can be removed after its S3 object has been verified. The helper
does not change the itch.io page's visibility or password protection.

To avoid a second local content archive, pass `-` as the output and pipe stdout
to `aws s3 cp - s3://<private-bucket>/inputs/<release>/project.tar.gz`. Use
`set -o pipefail` so failure on either side fails the transfer. Diagnostics go
to stderr; stdout contains only gzip data. Verify the resulting S3 object
before starting the Fargate task.

The packaging configuration explicitly includes MainMenu, Depot and
CentralBuilding. These maps are selected at runtime, so the default menu's
asset dependency closure is insufficient. The Mac helper also passes the map
list explicitly to UAT. If only the cook configuration needs correction,
update that configuration in the isolated snapshot and use `--resume <run>
--recook-only`; this reuses linked binaries after the object cleanup described
above. Validate actual packaged map startup, rather than treating UAT success
as evidence that every playable map is present.
