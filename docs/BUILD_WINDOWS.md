# Build Windows and Linux on Windows

Run from the **core** repository in 64-bit Windows PowerShell 5.1 or PowerShell
7. Close Unreal Editor and avoid editing assets while building. By default the
helper builds **Windows x64 and Linux x86_64**, then uploads both with Butler.

```powershell
# First run: prepare pinned private assets, build both platforms, upload both.
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-windows.ps1 -InitAssets

# Later runs: asset junctions, compilation/packaging/upload are automatic.
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-windows.ps1
```

The filename is retained for existing commands. `-ExecutionPolicy Bypass` is
limited to that PowerShell process. No administrator shell is needed by the
helper. The project is located relative to the script, independent of cwd.

## Dependencies

- **UE 5.8.3 Installed Build** from Epic Launcher, including **Windows and Linux
  target support**. The helper finds the engine using `UE_ENGINE_ROOT`, Epic's
  registry/Launcher manifest or standard installation paths; override with
  `-Engine 'D:\Epic Games\UE_5.8'`. It checks the engine version, Windows Editor,
  bundled .NET, build tools and Linux target module/base-game target receipt.
  Windows packaging with `-prereqs` requires `vc_redist.x64.exe` and
  `vc_redist.arm64.exe` under `Engine/Extras/Redist/en-us`. UE 5.8 stages both;
  the older `UEPrereqSetup_x64.exe` is not required by this version.
- **Visual Studio 2022/2026 or Build Tools**, C++ desktop/game tools, **MSVC x64
  and matching ATL**. Both platforms need the Windows Editor built for cooking.
  The helper reads `Engine/Config/Windows/Windows_SDK.json` for the minimum
  compiler and banned ranges instead of requiring one compiler family. For UE
  5.8.3, examples are **14.44.35211+ with VS 2022** or **14.50.35723+ with VS 2026**;
  older 14.44/14.50 patches and the 14.39–14.43 families are rejected by the engine.
  The minimum is 14.38.33130. Compiler product-version metadata is checked,
  falling back to its folder version, as in UnrealBuildTool. Detected incompatible
  or incomplete installations are listed with their paths and reasons.
  In Visual Studio Installer, use **Modify → Game development with C++** and
  **Individual components** to add/update the MSVC x64/x86 tools and corresponding
  C++ ATL. Discovery uses `vswhere`; no Developer PowerShell session is required.
  See [Epic's Visual Studio guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine).
- **Windows SDK 10.0.22621.0+**, including x64/UCRT headers and libraries,
  resource compiler and manifest tool.
- **Epic Linux cross-compile toolchain v26**, exactly
  **`v26_clang-20.1.8-rockylinux8`**. Use Epic's
  [Linux requirements/download page](https://dev.epicgames.com/documentation/en-us/unreal-engine/linux-development-requirements-for-unreal-engine).
  This compiles Linux games on Windows; a Linux VM or WSL is not required.
  After installing, reopen PowerShell so `LINUX_MULTIARCH_ROOT` is available.
  It must point to the version root containing `ToolchainVersion.txt` and
  `x86_64-unknown-linux-gnu`, not its `bin` subdirectory. Alternatively use
  `-LinuxToolchain 'C:\UnrealToolchains\v26_clang-20.1.8-rockylinux8'`.
  The engine's in-tree HostWin64/Linux SDK is a fallback when the variable is unset.
- **Python 3.9+** on PATH as `python.exe`, **Git**, **Git LFS**, and GitHub access
  to all three private asset repositories. `-InitAssets` invokes the existing
  asset setup only after tool checks pass; it fetches pinned revisions/LFS and
  creates Windows directory junctions. A normal build also creates/verifies
  junctions automatically when the asset files are already present, without
  fetching or switching submodule revisions. Commit local asset edits before switching
  revisions. Existing content is not overwritten by the asset helper.
  Python 3.9–3.11 recognize junctions through the Windows reparse tag; Python
  3.12+ uses `Path.is_junction`. An existing correct junction can be checked or
  mounted again without replacing it. Ordinary directories remain protected.
- **PCGExtendedToolkit 0.79.2 for UE 5.8 with Source**, plus all enabled plugins.
  Project/Marketplace code plugins need source for the requested targets. The
  helper copies their source into the external project, drops old binaries and
  intermediates, and sets `Installed=false` there so both platforms compile it.
  Installed engine/plugin descriptors are not changed.
- **Butler for Windows** on PATH, or pass `-Butler 'C:\Tools\butler.exe'`.
  [Install Butler](https://itch.io/docs/butler/installing.html) and run
  `butler login` once with an account allowed to publish this game. An alternate
  file is supported with `-ButlerIdentity 'D:\Private\itch-identity'`; authenticate
  it using `butler --identity 'D:\Private\itch-identity' login`. Keep identity
  files outside the repository. An existing secret `BUTLER_API_KEY` environment
  is also supported; keys are never passed as arguments or saved in receipts.

All selected targets are checked **before** any Unreal build starts. For Linux,
the helper checks the SDK version marker against the engine requirement,
compiler/linker/archive/objcopy tools, C++ headers and static libraries. It runs
`clang++ --version` and compiles/links a small C++20 program against the sysroot,
then validates its ELF64 little-endian x86_64 header. That Linux program is not
executed on Windows. Missing Linux prerequisites block a combined build; use
`-TargetPlatform Windows` when only Windows is wanted.

Other checks cover asset mounts/LFS pointers, branding/fonts, a closed Editor,
**10 GiB free on the source drive** and **60 GiB on the workspace drive**. These
are starting margins, not measured peaks. When both are on one drive, the
larger margin applies. Butler version and read-only `status` authentication are
checked unless `-NoUpload` is set. Software is not installed automatically.

## Options

```powershell
# Preflight only; includes a temporary Linux compiler smoke and online Butler status.
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-windows.ps1 -Check

# Windows only; Linux toolchain/support is not required.
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-windows.ps1 -TargetPlatform Windows

# Linux only, custom paths, Shipping; still builds the Windows Editor for cooking.
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-windows.ps1 -TargetPlatform Linux -Configuration Shipping -Engine 'D:\Epic Games\UE_5.8' -LinuxToolchain 'D:\Toolchains\v26_clang-20.1.8-rockylinux8' -OutputRoot 'D:\RR Builds'

# Both packages locally, without Butler or network authentication.
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-windows.ps1 -NoUpload
```

`-TargetPlatform Both` is the default. `-Check` cannot be combined with
`-InitAssets`; it never initializes assets, prepares a workspace, runs Unreal or
pushes a build, and does not create junctions. Add `-NoUpload` to skip its Butler
authentication check. A normal build mounts downloaded assets automatically
after the installed-tool checks pass; correct existing junctions are reused.

## Diagnosing preflight errors

Use the updated script and `setup-assets.py` together when upgrading from the
original Python 3.12-only helper. Multiple Python installations on PATH are
supported: the first matching executable is used, while Windows Store aliases
later on PATH are left alone. If a Store alias is first, move the real Python
installation before it on PATH. Python must be available as `python.exe` because
Unreal's version-stamping step calls that name too.

A missing redistributable does not mean Unreal was not found: the helper prints
the engine path before checking individual files. Verify the engine in Epic
Launcher only if the current `vc_redist` files or actual engine components are
missing. The old error about `UEPrereqSetup_x64.exe` was an obsolete check.

Likewise, `Cannot bind argument to parameter Path because it is null` in the
old plugin check was a script error: a Marketplace regex overwrote PowerShell's
automatic `$Matches` variable. The updated check retains the selected plugin
descriptor and reports missing source/version/platforms explicitly.

To isolate Windows prerequisites before installing Linux cross-compilation:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-windows.ps1 -TargetPlatform Windows -Check
```

For **Both**, enable the Linux platform in UE 5.8.3's Launcher installation
options and install Epic's v26 cross-compile toolchain separately. Reopen
PowerShell after installation and rerun `-Check` without the platform filter.

`[OK] Unreal Linux target support` confirms the Launcher component only; the
v26 cross-compiler is a separate installation. If already installed, inspect
`$env:LINUX_MULTIARCH_ROOT` or pass `-LinuxToolchain` pointing at its version root.
The root must contain `ToolchainVersion.txt` and `x86_64-unknown-linux-gnu`.

For `Missing mount: ...Content/...` during `-Check`, close Unreal and run the
normal build command: it now creates the links automatically after the other
tool checks pass. To prepare/verify links separately without starting a build:

```powershell
python .\scripts\setup-assets.py
python .\scripts\setup-assets.py --check
```

These commands validate sources/LFS first and refuse to overwrite existing data.
If the output instead reports missing submodule content or unresolved LFS
pointers, run `python .\scripts\setup-assets.py --init` with private GitHub access.
The packaging helper deliberately delays junction creation and `-InitAssets`
until other tool checks pass, so a missing Linux toolchain defers asset setup
on a combined run. Fix that prerequisite or use `-TargetPlatform Windows` for
Windows only. Existing directories and unexpected links remain protected.

Native stderr messages are displayed/logged as plain text; a Python validation
error still returns a failing exit code, without PowerShell's extra
`NativeCommandError`/`CategoryInfo` formatting obscuring its actual message.

## Workspace, outputs and failures

The helper copies source/configuration/branding into an external build project
and stores the original HEAD in `Build/SourceCommit.txt` for version stamping.
**Content is shared through a directory junction**, so this is not an immutable
asset snapshot: do not edit/update assets during the run. Generated binaries,
caches and converted plugin descriptors stay in the external workspace. The
script never uploads source, logs, receipts or the engine. Unreal may also use
its standard global DDC/Zen caches outside the workspace.

```text
../builds/desktop/<timestamp>-<unique-id>/
  project/                # build project, shared Content, copied source plugins
  editor.log              # workspace preparation and one Windows Editor build
  summary.json            # original run result and selected platforms
  windows/
    dist/Windows/         # staged Windows game
    build.log
    receipt.json
    upload.log
    itch-upload.json
  linux/
    dist/Linux/           # staged Linux game
    build.log
    receipt.json
    upload.log
    itch-upload.json
```

Archive subdirectory names are determined by UAT; the helper discovers the
staged launcher root. First it compiles `RespawnAndRageEditor` once for Win64
Development, then builds/cooks the selected Game targets in the chosen
configuration (Development by default). Maps are Main Menu, Depot and Central
Building. Packaging is sequential; do not run concurrent builds on a workspace.

Each successful package is checked against its own stamped game version.
Linux additionally requires a real ELF x86_64 game executable; shell scripts
are normalized to UTF-8/LF. Uploads use the same version as the game menu and
the fixed channels `aaggeeaa/respawn-and-rage:windows-x86_64` and
`aaggeeaa/respawn-and-rage:linux-x86_64`. Linux pushes explicitly enable Butler's
`--fix-permissions` to restore executable bits when publishing from NTFS.

A failed shared Editor build stops both targets. A failed platform package or
upload is recorded separately; the other platform can still complete and
upload. Any failure returns exit code 1, while complete success returns 0.
After a successful Butler upload, that platform's entire attempt directory is
removed, including package, logs and receipts. Once all selected platforms have
uploaded, the whole run directory is deleted too, including the shared project,
Editor log and summary. Failed/pending attempts and the shared workspace needed
to retry them remain; `-NoUpload` and `-Check` do not trigger cleanup. Deletion
uses Python's junction-safe `rmtree` so source Content mounts are preserved.
Until cleanup, `receipt.json` records each platform's state: `packaged`,
`uploaded`, `failed`, or `upload_failed`.

## Retry uploads without recompiling

Pass the **run directory** to retry pending packaged/upload_failed targets,
skipping successful attempts already removed, or a **platform directory** to
retry that package. Python is required for automatic cleanup; no Unreal, compiler
or asset setup is required:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-windows.ps1 -UploadOnly 'D:\RR Builds\20261020-150000-12345678'
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-windows.ps1 -UploadOnly 'D:\RR Builds\20261020-150000-12345678\linux'
```

Use `-TargetPlatform Linux`/`Windows` to filter a run-directory retry. Add `-Check`
to validate pending packages/authentication without pushing. `-UploadOnly`
cannot be combined with `-NoUpload`/`-InitAssets`. Legacy Windows-only attempt
directories are supported; receipts without a platform are treated as Windows.
Retries update platform/upload receipts, then remove successful attempts. When
all original platforms have completed, the parent run and summary are removed too. Butler success does not verify runtime startup or
completion of itch.io processing. Existing page access settings are retained.

Test launch, menus, maps and LAN on each target OS before treating a package as
validated. Native Windows cross-compilation and real uploads still need testing;
the automated helper tests use fixtures/mocked tools on macOS. Run them with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\tests\test_package_windows.ps1
python -m unittest discover -s scripts/tests -p test_prepare_desktop_build.py
```
