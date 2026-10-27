# Local macOS builds

To package Windows/Linux x64 on Windows, see [the desktop build guide](BUILD_WINDOWS.md)
and `scripts/package-windows.ps1`.

Use these commands from the game repository. The Mac build uses the locally
installed Unreal Engine and Xcode directly. It needs no separate builder
repository, container, AWS account or CI runner.

## One-command Mac release

After every successful Butler upload, `release-macos.py` automatically deletes
that release's entire directory: isolated project, app, Xcode archive, scratch
files, test output, logs and receipts. This also applies to `--resume` and
`--upload-only`. Cleanup starts only after Butler exits successfully and its
log handle is closed. Failed builds/uploads, `--check` and `--no-upload` keep
their files for diagnosis or retry. The source project and sibling runs are
preserved. Upload success means Butler accepted the build; itch may still be
processing it. A cleanup error is reported without changing the upload result
stored in any remaining receipt.

New Mac builds first remove payloads from previous completed builds, then measure
actual free disk space before checking the build requirements. Both entry points
inspect the selected workspace, `../builds/macos` and the legacy temporary
workspace `/private/tmp/rr-sm5-releases`. Uploaded releases and completed standalone
packages lose their project snapshots, packaged apps and temporary diagnostics or
Xcode archives; small logs and receipts remain. Failed/running releases and
releases awaiting upload are preserved. `--resume` and `--upload-only` skip cleanup;
`--check` only previews it and reports the current free space, without assuming
that APFS shared files will free their full apparent size.

`release-macos.py` runs cleanup, dependency/space checks, the isolated build, Developer ID
signing, Apple notarization, ticket stapling, Gatekeeper assessment and Butler
upload in that order. Run it from this repository on a Mac:

```sh
# Read-only preflight (including online authentication checks):
python3 scripts/release-macos.py --check

# Build, sign, notarize and upload the universal app:
python3 scripts/release-macos.py
```

Before the first release, install the prerequisites below and a valid **Developer
ID Application certificate with its private key** in Keychain. The script finds
[Butler](https://itch.io/docs/butler/installing.html) on PATH, in the itch app's
bundled versions, or in the existing sibling builder's `.work/butler/butler`.
It also finds the existing login in the normal Mac/Linux-style Butler locations
or `../unreal-builder/.work/itch-identity`. No separate builder repo is required;
those legacy locations are optional migration fallbacks. Explicit `--butler` and
`--butler-identity` values take precedence. Keys are never read into the script
or printed; Butler reads the selected identity file itself.

For notarization, the default `--notarization-method auto` checks the notarytool
profile first, then uses the Apple Developer account already configured in
**Xcode Settings → Accounts**. This uses Xcode's supported Developer ID archive
upload and notarized export commands; it never extracts Xcode tokens/passwords.
An account listed in Xcode can still need its login refreshed if Apple rejects
authentication at upload time. The Xcode preflight checks account configuration;
only the upload validates its authentication. A notarytool profile also supports
an authenticated, read-only preflight through `notarytool history`.

To use notarytool explicitly, configure its credentials once in interactive
prompts, keeping passwords out of the repository and command history:

```sh
xcrun notarytool store-credentials RespawnAndRage
butler login
```

An existing Butler login is reused; run `butler login` only when none is present.
The script auto-selects the signing identity only when exactly one valid
Developer ID Application identity exists. Otherwise supply
`--identity 'Developer ID Application: Your Name (TEAMID)'`.
`--keychain-profile NAME` selects a different existing notarytool profile.
`--notarization-method notarytool` requires that profile; `--notarization-method
xcode` explicitly uses the configured Xcode account.
Missing tools/credentials stop the release before compilation; software and
certificates are not installed automatically. The initial checks include UE
5.8.3, full Xcode/SDK/Metal, bundled .NET, plugin source, branding, asset mounts/LFS,
a closed Editor, signing credentials and Butler authentication. If Metal is
installed but its Xcode asset mapping/mount is stale, the check refreshes the
installed component with `xcodebuild -showComponent MetalToolchain` and retries
`xcrun --sdk macosx metal -v`. It does not download or reinstall Metal.

The full release reserves **30 GiB for universal**, **25 GiB for a single
architecture** on APFS, or **45 GiB for ordinary copies**, plus **5 GiB on the
source and user-home volumes** for shared caches/tools (one margin when they
share a volume). These are starting margins, not
guaranteed peaks. After packaging, the script requires the app's uncompressed
size plus 1 GiB free for notarytool, or twice the app size plus 1 GiB for the
Xcode archive/export route. Temporary notarization files stay on the selected
workspace disk, including when using an external SSD. Xcode archives are retained
beside the build receipt for diagnosis until a successful itch upload; the final upload still contains only the
verified `.app`.

```sh
# Use a larger build disk, an explicit identity and Shipping configuration:
python3 scripts/release-macos.py \
  --workspace '/Volumes/BuildSSD/RespawnAndRage/mac' \
  --identity 'Developer ID Application: Your Name (TEAMID)' \
  --configuration Shipping

# Fetch pinned private assets and create mounts before building:
python3 scripts/release-macos.py --init-assets

# Produce a signed, notarized app without uploading:
python3 scripts/release-macos.py --no-upload

# Reuse a completed package after a signing/notarization failure:
python3 scripts/release-macos.py --resume '/absolute/path/to/release-TIMESTAMP-ID'

# Retry upload without building, signing or submitting again:
python3 scripts/release-macos.py --upload-only '/absolute/path/to/release-TIMESTAMP-ID'
```

`--init-assets` cannot be combined with `--check` or either retry mode. A failed
build needs a fresh release; `--resume` requires a successfully packaged build.
Both retry modes use that build's recorded architecture and version, not current
source changes. `--upload-only --check` verifies the package, signature, ticket,
Gatekeeper and Butler access without pushing. Every upload repeats those checks;
an old success receipt alone cannot authorize a modified or unsigned bundle.

Universal uploads go to `aaggeeaa/respawn-and-rage:mac-universal`. The optional
`--architecture arm64` and `--architecture x64` use `mac-arm64` and `mac-x86_64`
respectively. Butler receives the version embedded in the app. Only the directory
containing the single final `.app` is pushed. Logs, sources and receipts stay out
of that directory; existing itch page access settings are retained.

`--butler /absolute/path/to/butler` supports non-PATH installations, and
`--butler-identity /private/path/to/identity` selects an alternate login file.
Authenticate it with `butler --identity /private/path/to/identity login` first.
`BUTLER_API_KEY` in the environment is also supported; secrets are never passed
as command arguments or written to receipts.

```text
../builds/macos/release-TIMESTAMP-ID/
  # This whole directory is removed after a successful itch upload.
  release.json                 # current stage, version, channel, errors
  release.log                  # package helper output
  notarization.log
  upload.log
  scratch/                     # temporary ZIP, removed after notarization
  build/
    receipt.json
    build.log                  # detailed Unreal output; tail -f while building
    RespawnAndRage-notarization.json
    RespawnAndRage.xcarchive    # Xcode notarization route; retained for diagnostics
    dist/RespawnAndRage.app
    project/                   # isolated source/assets and intermediates
```

The script prints the release directory before building. Keep it for retries;
failed stages return exit code 1 and preserve the package. Gatekeeper acceptance
does not validate gameplay. Check a browser-downloaded copy on another Mac,
including Intel execution for a universal release.

## Prerequisites

- Unreal Engine **5.8.3**, normally at `/Users/Shared/Epic Games/UE_5.8`.
- Full Xcode selected as the active developer directory, with its license and
  first-launch setup completed. Xcode 27.1 was used for the first package.
- Python **3.10 or newer** and Git.
- EOS guest play requires the generated project plugin. Build/open and isolated
  packaging helpers prepare it automatically from the installed UE sources;
  before a manual IDE/UBT build run `python3 scripts/prepare-eos-guest.py --engine
  "/path/to/UE_5.8"`. Keep `Plugins/OnlineSubsystemEOS` ignored: it contains
  locally copied Epic source. See `EOS_ONLINE.md`.
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
Mac packages cook both Metal SM6 and SM5 shaders, retaining the SM5 fallback
for Macs that cannot select SM6. CPU architecture alone does not establish GPU
compatibility; verify both shader paths on the actual cooked app before upload.
It creates an isolated project snapshot and follows the private Content mounts.
Compilation and cooking take place in that copy, preserving the live checkout.
Only the runtime background and icon are included from Branding; source artwork
under Branding/Originals is not shipped.

Packaging uses the single `MapsToCook` list in `Config/DefaultGame.ini`; do not
pass a separate `-map` list, which overrides those settings and can silently omit
new menu maps. This applies to the Mac and Windows/Linux packaging helpers.

Development Mac packages boot every configured gameplay map offline and as a
listen server with six bots, including the pistol, before accepting the package.
Each launch must load the requested world without travel failures; listen cases
must also start the game net driver. Results are recorded in `runtime-smoke/maps.json`. This checks the native
rendering profile and animation evaluation; starting the main menu alone misses
these failures. Both SM5 and SM6 also run with real offscreen Metal rendering.
These checks require an SM6-capable build Mac (M2+ with macOS 15+), and verify
the shader platform actually selected by the game.
The logs and receipts are saved under `runtime-smoke/` in the build
directory. To repeat it, run:

```sh
python3 scripts/test-packaged-runtime.py --executable /path/to/RespawnAndRage.app/Contents/MacOS/RespawnAndRage
# Check every gameplay map offline and as a listen server:
python3 scripts/test-packaged-runtime.py --executable /path/to/RespawnAndRage.app/Contents/MacOS/RespawnAndRage --all-maps
# Optional real Metal rendering and FPS samples:
python3 scripts/test-packaged-runtime.py --executable /path/to/RespawnAndRage.app/Contents/MacOS/RespawnAndRage --render
# Force the compatibility path (use sm6 for the other shader path):
python3 scripts/test-packaged-runtime.py --executable /path/to/RespawnAndRage.app/Contents/MacOS/RespawnAndRage --render --shader-model sm5
```

The game applies AA2/GI0/reflections0/shadows1 in native startup for both local
and packaged execution. `play.command` no longer supplies a separate quality
profile. The existing 60 FPS resolution governor and 1080p scene cap remain.

FPS clips must use compressed playback with normal retargeting. Do not call
`FBoneContainer::SetDisableRetargeting` in gameplay: UE 5.8.3 treats it as a raw
data debugging mode and cooked animations assert during curve evaluation.
`scripts/bake_cooked_first_person.py`, run through Unreal Python, migrates the
owned pelvis/right-shoulder translation tracks once, backs up the original
packages privately, and preserves the effective poses of the other weapons.
It verifies every animation frame and compressed pistol playback on all twenty
skins. Subsequent runs verify the migration; `-ERFPSCookVerify` requires its
saved marker. Never regenerate the original raw-data workaround.
New Mac packages run this migration/verifier in the isolated snapshot after
compiling the Editor, before cooking. The pistol's three clips use the owned
`BC_ER_PistolPrecise` settings with full quaternion storage, preserving its
strict barrel alignment without requiring editor-only raw data.

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

Successful itch uploads now delete the entire release directory automatically,
including all logs and receipts. The following manual instructions apply to
standalone packages that have not been uploaded.

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

The release command can reuse the existing Xcode account automatically. For
manual helper use with that account (no notarytool profile required):

```sh
python3 scripts/notarize-macos.py /absolute/path/to/build/dist/RespawnAndRage.app \
  --identity 'Developer ID Application: Your Name (TEAMID)' --method xcode
```

This creates an APFS-cloned xcarchive, submits it with Xcode's Developer ID
distribution workflow, waits for its notarized export, and verifies the ticket,
signature, version and Gatekeeper before replacing the packaged app. No account
password/token is extracted. Failed archives remain beside `dist`; the helper
waits up to 30 minutes for the export (`--timeout-seconds` changes that limit).

Alternatively, create a notarytool Keychain profile locally; enter credentials
only in its interactive prompts, never in source files or chat:

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

The one-command release above performs these steps and the Butler upload without
the separate builder repository. For manual packaging/notarization, run a
packaged Metal startup/gameplay check before publishing the unchanged app. Do not modify
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
The game build and Developer ID signature can already be valid at this point.
Keep the release directory printed by the command, sign in again in **Xcode
Settings → Accounts**, then retry without rebuilding:

```sh
python3 scripts/release-macos.py --resume '/absolute/path/to/release-TIMESTAMP-ID'
```

This retries signing/notarization and uploads only after ticket and Gatekeeper
verification. The failed notarization receipt records the error and retained
archive path; existing failed archives are preserved when retrying.

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

Release source identity: Mac and Windows releases require a clean Git working tree,
including untracked files and dirty submodules (also for resume/upload retries).
Commit or stash changes before running them. GameVersion uses the HEAD commit
committer date in its recorded timezone, followed by the eight-character hash;
isolated workspaces preserve that date in Build/SourceCommitDate.txt. Operational
log timestamps and build directory names still use the actual execution time.
