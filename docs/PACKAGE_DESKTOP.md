# Windows and Linux packaging

Isolated build projects are written under `../builds/`.

**Code:** `scripts/package-windows.ps1`, `archive-linux-project.py`.

## Windows and Linux (from Windows)

Run from `core` in 64-bit PowerShell, with Unreal closed. The default builds Windows
x64 and Linux x86_64 and uploads both. `-InitAssets` is only needed the first time.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-windows.ps1 -InitAssets
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-windows.ps1
```

| Option | Effect |
| --- | --- |
| `-Check` | Preflight only (includes a Linux compiler smoke and Butler status). Not with `-InitAssets`. |
| `-TargetPlatform Windows\|Linux` | One platform; Linux still builds the Windows Editor for cooking. |
| `-NoUpload` | Keep packages local, skip Butler and its authentication. |
| `-Configuration Shipping` | Default is Development. |
| `-Engine`, `-LinuxToolchain`, `-OutputRoot`, `-Butler`, `-ButlerIdentity` | Override discovered paths. |
| `-UploadOnly <run or platform dir>` | Retry pending uploads without compiling; filter with `-TargetPlatform`, validate with `-Check`. |

Requirements:

- UE 5.8.3 Installed Build with **Windows and Linux target support**. Found through
  `UE_ENGINE_ROOT`, Epic's registry or standard paths.
- Visual Studio 2022/2026 or Build Tools with C++ desktop/game tools, MSVC x64 and
  matching ATL. The helper reads `Engine/Config/Windows/Windows_SDK.json` for allowed
  compilers (for UE 5.8.3: 14.44.35211+ with VS 2022 or 14.50.35723+ with VS 2026).
- Windows SDK 10.0.22621.0+.
- Epic Linux cross toolchain **`v26_clang-20.1.8-rockylinux8`**.
  `LINUX_MULTIARCH_ROOT` must point to the version root that contains
  `ToolchainVersion.txt`, not its `bin` folder. No VM or WSL is needed.
- Butler on PATH with one `butler login`; `BUTLER_API_KEY` is also accepted.

Every selected target is checked before Unreal starts: tools, asset mounts and LFS
pointers, a closed Editor, 10 GiB free on the source drive and 60 GiB on the workspace
drive. Software is never installed automatically.

Content is shared through a junction, so do not edit assets during a run. Output goes
to `../builds/desktop/<timestamp>-<id>/{windows,linux}/` with `build.log`, `receipt.json`
and `upload.log`. Linux must produce a real ELF x86_64 executable and pushes with
`--fix-permissions`. A failed platform is recorded separately while the other can
finish; any failure returns exit code 1. Successful uploads delete their attempt
directory, and the whole run once all platforms have uploaded.

Common preflight problems:

- `Missing mount: ...Content/...`: run `python .\scripts\setup-assets.py`, or just the
  normal build, which mounts assets after the tool checks pass.
- `[OK] Unreal Linux target support` covers only the Launcher component; the v26
  toolchain is a separate installation.
- A Windows Store Python alias first on PATH must be moved behind real Python.

Helper tests:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\tests\test_package_windows.ps1
python -m unittest discover -s scripts/tests -p test_prepare_desktop_build.py
```

## Linux through Fargate

`unreal-builder` builds Linux on AWS. The cached engine lacks Fab plugins, so create
the private input with the helper, which adds them as source-only plugins:

```sh
python3 scripts/archive-linux-project.py /private/tmp/rr-linux-project.tar.gz --check
python3 scripts/archive-linux-project.py /private/tmp/rr-linux-project.tar.gz
```

Pass `-` as output to stream into `aws s3 cp - s3://<bucket>/inputs/<release>/project.tar.gz`
with `set -o pipefail`. Then run `unreal-builder/scripts/run-task.py` with `--project-key`
and `--publish-itch`. Never commit that archive. See `../unreal-builder/docs/LINUX_BUILD.md`.
