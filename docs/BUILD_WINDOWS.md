# Build Windows and Linux on Windows

Run from the **core** repository in 64-bit Windows PowerShell 5.1 or PowerShell
7. Close Unreal Editor and avoid editing assets while building. By default the
helper builds **Windows x64 and Linux x86_64**, then uploads both with Butler.

```powershell
# First run: prepare pinned private assets, build both platforms, upload both.
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-windows.ps1 -InitAssets

# Later runs: assets are checked, compilation/packaging/upload are automatic.
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
- **Visual Studio 2026 or Build Tools**, C++ desktop/game tools, **MSVC 14.50
  x64/x86** (compiler product version **14.50.35723.0+**) and matching **ATL**.
  Both platforms need the Windows Editor built for cooking. Visual Studio is
  located with `vswhere`; a Developer PowerShell session is not required.
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
- **Python 3.12+** on PATH as `python.exe`, **Git**, **Git LFS**, and GitHub access
  to all three private asset repositories. `-InitAssets` invokes the existing
  asset setup only after tool checks pass; it fetches pinned revisions/LFS and
  creates Windows directory junctions. Commit local asset edits before switching
  revisions. Existing content is not overwritten by the asset helper.
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
pushes a build. Add `-NoUpload` to skip its Butler authentication check.

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
Completed packages are retained. `receipt.json` is authoritative for each
platform: `packaged`, `uploaded`, `failed`, or `upload_failed`.

## Retry uploads without recompiling

Pass the **run directory** to retry pending packaged/upload_failed targets,
skipping those already uploaded, or a **platform directory** to explicitly
re-upload that package. No Unreal, Python, compiler or asset setup is required:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-windows.ps1 -UploadOnly 'D:\RR Builds\20261020-150000-12345678'
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-windows.ps1 -UploadOnly 'D:\RR Builds\20261020-150000-12345678\linux'
```

Use `-TargetPlatform Linux`/`Windows` to filter a run-directory retry. Add `-Check`
to validate pending packages/authentication without pushing. `-UploadOnly`
cannot be combined with `-NoUpload`/`-InitAssets`. Legacy Windows-only attempt
directories are supported; receipts without a platform are treated as Windows.
Retries update platform/upload receipts; the original `summary.json` remains a
record of the initial run. Butler success does not verify runtime startup or
completion of itch.io processing. Existing page access settings are retained.

Test launch, menus, maps and LAN on each target OS before treating a package as
validated. Native Windows cross-compilation and real uploads still need testing;
the automated helper tests use fixtures/mocked tools on macOS. Run them with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\tests\test_package_windows.ps1
python -m unittest discover -s scripts/tests -p test_prepare_desktop_build.py
```
