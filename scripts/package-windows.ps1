#requires -Version 5.1
<#
.SYNOPSIS
Build Windows and Linux x64 on Windows, then upload both with Butler.
.EXAMPLE
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-windows.ps1 -InitAssets
.EXAMPLE
.\scripts\package-windows.ps1 -Check -Engine 'D:\Epic Games\UE_5.8'
.NOTES
Uses an external build workspace with shared Content; close Unreal Editor first.
Uploads with Butler unless -NoUpload is passed. -Check never builds or uploads.
#>
[CmdletBinding()]
param(
    [string]$Engine,
    [ValidateSet('Development', 'Shipping')][string]$Configuration = 'Development',
    [string]$OutputRoot,
    [switch]$InitAssets,
    [switch]$Check,
    [string]$Butler = 'butler.exe',
    [string]$ButlerIdentity,
    [switch]$NoUpload,
    [string]$UploadOnly,
    [ValidateSet('Both', 'Windows', 'Linux')][string]$TargetPlatform = 'Both',
    [string]$LinuxToolchain
)

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
$projectRoot = Split-Path $PSScriptRoot -Parent
$uproject = Join-Path $projectRoot 'RespawnAndRage.uproject'
$problems = New-Object 'System.Collections.Generic.List[string]'
$logFile = $null
$resolvedEngine = $null
$previousLinuxRoot = $env:LINUX_MULTIARCH_ROOT
$platforms = @('Windows', 'Linux')
if ($TargetPlatform -ne 'Both') { $platforms = @($TargetPlatform) }

function Test-Dependency([string]$Name, [scriptblock]$Action) {
    try {
        & $Action
        Write-Host "[OK] $Name" -ForegroundColor Green
    } catch {
        $problems.Add("${Name}: $($_.Exception.Message)")
        Write-Host "[MISSING] ${Name}: $($_.Exception.Message)" -ForegroundColor Red
    }
}

function Invoke-Native([string]$FilePath, [string[]]$Arguments, [switch]$Capture) {
    # Resolve before relaxing ErrorActionPreference: a missing executable must
    # never reuse the exit code from an earlier successful native command.
    $command = Get-Command -Name $FilePath -CommandType Application -ErrorAction Stop
    # Windows PowerShell 5.1 turns native stderr into ErrorRecords. Keep both
    # streams visible and use the process exit code, including through Tee-Object.
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        if ($Capture) {
            $output = @(& $command.Source @Arguments 2>&1)
            $output | Out-Host
            if ($logFile) { $output | Out-File -LiteralPath $logFile -Append }
        } elseif ($logFile) {
            & $command.Source @Arguments 2>&1 | Tee-Object -FilePath $logFile -Append | Out-Host
        } else {
            & $command.Source @Arguments 2>&1 | Out-Host
        }
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousPreference
    }
    if ($code -ne 0) { throw "$FilePath failed (exit $code). See the output above." }
    if ($Capture) { return ($output -join "`n") }
}

function Find-Engine {
    if ($Engine) { return $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Engine) }
    $candidates = @($env:UE_ENGINE_ROOT)
    $registered = Get-ItemProperty 'HKLM:\SOFTWARE\EpicGames\Unreal Engine\5.8' -ErrorAction SilentlyContinue
    if ($registered) { $candidates += $registered.InstalledDirectory }
    $launcherFile = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
    if (Test-Path -LiteralPath $launcherFile) {
        $launcher = Get-Content -LiteralPath $launcherFile -Raw | ConvertFrom-Json
        $candidates += @($launcher.InstallationList | Where-Object AppName -eq 'UE_5.8' | ForEach-Object InstallLocation)
    }
    $candidates += @('C:\Unreal\UE_5.8', (Join-Path $env:ProgramFiles 'Epic Games\UE_5.8'))
    foreach ($candidate in $candidates) {
        if (-not $candidate) { continue }
        $versionFile = Join-Path $candidate 'Engine\Build\Build.version'
        if (Test-Path -LiteralPath $versionFile) {
            $version = Get-Content -LiteralPath $versionFile -Raw | ConvertFrom-Json
            if ("$($version.MajorVersion).$($version.MinorVersion).$($version.PatchVersion)" -eq '5.8.3') {
                return [IO.Path]::GetFullPath($candidate)
            }
        }
    }
    throw 'Install UE 5.8.3 in Epic Launcher, or specify -Engine ''D:\Epic Games\UE_5.8''.'
}

function Assert-FreeSpace([string]$Path, [double]$MinimumGiB) {
    $existing = $Path
    while (-not (Test-Path -LiteralPath $existing)) {
        $parent = Split-Path $existing -Parent
        if (-not $parent -or $parent -eq $existing) { throw "No accessible local drive for $Path." }
        $existing = $parent
    }
    $drive = (Get-Item -LiteralPath $existing).PSDrive
    if ($null -eq $drive.Free) { throw "Cannot check free space on $Path. Use a local drive." }
    if ($drive.Free -lt $MinimumGiB * 1GB) {
        throw ('Need at least {0} GiB free on {1}; available: {2:N1} GiB.' -f $MinimumGiB, $drive.Root, ($drive.Free / 1GB))
    }
}

function Invoke-EditorBuild([string]$EngineRoot, [string]$ProjectPath) {
    Invoke-Native (Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat') @(
        'RespawnAndRageEditor', 'Win64', 'Development', "-Project=$ProjectPath", '-NoHotReloadFromIDE', '-WaitMutex')
}

function Invoke-PackageBuild([string]$EngineRoot, [string]$ProjectPath, [string]$ArchivePath, [string]$BuildConfiguration,
        [ValidateSet('Windows', 'Linux')][string]$Platform = 'Windows') {
    $unrealPlatform = 'Win64'
    if ($Platform -eq 'Linux') { $unrealPlatform = 'Linux' }
    $extra = @('-prereqs')
    if ($Platform -eq 'Linux') { $extra = @('-clientarchitecture=x64') }
    Invoke-Native (Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat') (@(
        'BuildCookRun', "-project=$ProjectPath", '-target=RespawnAndRage', "-platform=$unrealPlatform",
        "-clientconfig=$BuildConfiguration", '-nop4', '-unattended', '-utf8output', '-installed',
        '-build', '-cook', '-stage', '-package', '-pak', '-iostore', '-skipbuildeditor',
        '-map=/Game/EclipseRecoil/Maps/L_MainMenu+/Game/EclipseRecoil/Maps/L_Depot+/Game/EclipseRecoil/Maps/L_CentralBuilding',
        '-archive', "-archivedirectory=$ArchivePath") + $extra)
}

function Assert-Elf64([string]$Path) {
    $stream = [IO.File]::OpenRead($Path)
    try { $header = New-Object byte[] 20; $count = $stream.Read($header, 0, 20) } finally { $stream.Dispose() }
    if ($count -ne 20 -or $header[0] -ne 127 -or $header[1] -ne 69 -or $header[2] -ne 76 -or $header[3] -ne 70 -or
            $header[4] -ne 2 -or $header[5] -ne 1 -or [BitConverter]::ToUInt16($header, 16) -notin 2, 3 -or [BitConverter]::ToUInt16($header, 18) -ne 62) {
        throw "Expected a Linux x86_64 ELF binary: $Path"
    }
}

function Assert-LinuxToolchain([string]$EngineRoot) {
    $sdk = Get-Content -LiteralPath (Join-Path $EngineRoot 'Engine\Config\Linux\Linux_SDK.json') -Raw | ConvertFrom-Json
    $requiredVersion = 'v26_clang-20.1.8-rockylinux8'
    if ($sdk.MainVersion -ne $requiredVersion) { throw "Unexpected engine Linux SDK requirement: $($sdk.MainVersion). UE 5.8.3/v26 required." }
    $root = $LinuxToolchain
    if (-not $root) { $root = $env:LINUX_MULTIARCH_ROOT }
    if (-not $root) { $root = Join-Path $EngineRoot "Engine\Extras\ThirdPartyNotUE\SDKs\HostWin64\Linux\$requiredVersion" }
    $root = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($root)
    $marker = Join-Path $root 'ToolchainVersion.txt'
    if (-not (Test-Path -LiteralPath $marker) -or (Get-Content -LiteralPath $marker -Raw).Trim() -ne $requiredVersion) {
        throw "Install Epic $requiredVersion, reopen PowerShell, and check LINUX_MULTIARCH_ROOT or pass -LinuxToolchain 'C:\UnrealToolchains\$requiredVersion'."
    }
    $arch = Join-Path $root 'x86_64-unknown-linux-gnu'
    foreach ($file in @('bin\clang++.exe', 'bin\ld.lld.exe', 'bin\llvm-ar.exe', 'bin\llvm-objcopy.exe',
            'include\c++\v1\vector', 'lib64\libc++.a', 'lib64\libc++abi.a')) {
        if (-not (Test-Path -LiteralPath (Join-Path $arch $file))) { throw "Incomplete Linux toolchain: missing $file in $arch." }
    }
    $compiler = Join-Path $arch 'bin\clang++.exe'
    $compilerVersion = Invoke-Native $compiler @('--version') -Capture
    if ($compilerVersion -notmatch 'clang version 20\.1\.8\b') { throw 'Linux toolchain must run clang 20.1.8.' }
    $temporary = Join-Path ([IO.Path]::GetTempPath()) ('rr-linux-toolchain-' + [Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $temporary | Out-Null
    try {
        $source = Join-Path $temporary 'smoke.cpp'
        $binary = Join-Path $temporary 'smoke'
        '#include <vector>', 'static_assert(sizeof(void*) == 8);', 'int main() { std::vector<int> v{42}; return v[0] == 42 ? 0 : 1; }' |
            Set-Content -LiteralPath $source -Encoding ASCII
        Invoke-Native $compiler @('--target=x86_64-unknown-linux-gnu', "--sysroot=$arch", '-std=c++20', '-fuse-ld=lld',
            '-nostdinc++', '-isystem', (Join-Path $arch 'include'), '-isystem', (Join-Path $arch 'include\c++\v1'),
            "-L$(Join-Path $arch 'lib64')", $source, '-o', $binary, '-nodefaultlibs',
            (Join-Path $arch 'lib64\libc++.a'), (Join-Path $arch 'lib64\libc++abi.a'), '-lm', '-lc', '-lpthread', '-lgcc_s', '-lgcc')
        Assert-Elf64 $binary
    } finally { Remove-Item -LiteralPath $temporary -Recurse -Force }
    # Match the path UBT will use, including an explicit -LinuxToolchain override.
    $env:LINUX_MULTIARCH_ROOT = $root
    Write-Host "Linux toolchain: $root (C++20 link and ELF x86_64 verified)"
}

function Assert-LinuxSupport([string]$EngineRoot) {
    $modules = @(Get-ChildItem (Join-Path $EngineRoot 'Engine\Binaries\Win64') -Recurse -Filter 'UnrealEditor-LinuxTargetPlatform.dll' -File)
    $target = 'UnrealGame.target'
    if ($Configuration -eq 'Shipping') { $target = 'UnrealGame-Linux-Shipping.target' }
    if (-not $modules.Count -or -not (Test-Path -LiteralPath (Join-Path $EngineRoot "Engine\Binaries\Linux\$target"))) {
        throw 'In Epic Launcher, enable Linux target support for UE 5.8.3 and verify the engine installation.'
    }
}

function Get-ButlerArguments {
    if ($ButlerIdentity) {
        '--identity'
        $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ButlerIdentity)
    }
}

function Assert-Butler {
    try { Invoke-Native $Butler @('version') } catch {
        throw 'Install Butler for Windows and add it to PATH, or pass -Butler ''C:\Tools\butler.exe''. See https://itch.io/docs/butler/installing.html.'
    }
    # Check presence only; never read or print the secret. Avoid triggering an
    # interactive login in the middle of an unattended build.
    if ([string]::IsNullOrWhiteSpace($env:BUTLER_API_KEY)) {
        $identity = Join-Path $env:USERPROFILE '.config\itch\butler_creds'
        if ($ButlerIdentity) { $identity = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ButlerIdentity) }
        if (-not (Test-Path -LiteralPath $identity -PathType Leaf) -or (Get-Item -LiteralPath $identity).Length -eq 0) {
            throw 'Run butler login first (with --identity PATH if using -ButlerIdentity), or supply BUTLER_API_KEY through your environment.'
        }
    }
    try {
        # Read-only API check; the Windows channel need not exist yet.
        Invoke-Native $Butler (@(Get-ButlerArguments) + @('status', 'aaggeeaa/respawn-and-rage'))
    } catch {
        throw 'Butler could not access aaggeeaa/respawn-and-rage. Check your connection and butler login credentials.'
    }
}

function Get-GamePackage([string]$ArchivePath, [string]$ExpectedVersion,
        [ValidateSet('Windows', 'Linux')][string]$Platform = 'Windows') {
    # UAT normally archives into dist/Windows. Select the staged launcher root,
    # excluding the second executable under RespawnAndRage/Binaries/Win64.
    $launcherName = 'RespawnAndRage.exe'
    if ($Platform -eq 'Linux') { $launcherName = 'RespawnAndRage.sh' }
    $launchers = @(Get-ChildItem -LiteralPath $ArchivePath -Recurse -File -Filter $launcherName | Where-Object {
        Test-Path -LiteralPath (Join-Path $_.DirectoryName 'RespawnAndRage\Content\Paks') -PathType Container
    })
    if ($launchers.Count -ne 1) { throw "Expected one staged $Platform game with $launcherName and cooked Content/Paks." }
    $directory = $launchers[0].DirectoryName
    $stamp = Join-Path $directory 'RespawnAndRage\Build\GameVersion.txt'
    $version = (Get-Content -LiteralPath $stamp -Raw).Trim()
    if ($version -notmatch '^\d{2}\.\d{2}\.\d{2}\.[0-9a-f]{8}$') { throw 'Invalid packaged game version.' }
    if ($ExpectedVersion -and $version -ne $ExpectedVersion) { throw 'Packaged game version differs from this build.' }
    $containers = @(Get-ChildItem -LiteralPath (Join-Path $directory 'RespawnAndRage\Content\Paks') -File | Where-Object Extension -in '.pak', '.utoc')
    if (-not $containers.Count) { throw 'Cooked game containers are missing.' }
    if ($Platform -eq 'Linux') {
        $binaries = @(Get-ChildItem -LiteralPath (Join-Path $directory 'RespawnAndRage\Binaries\Linux') -File | Where-Object {
            $_.Name -match '^RespawnAndRage(?:-Linux-(?:Development|Shipping))?$'
        })
        if ($binaries.Count -ne 1) { throw 'Expected one Linux game executable.' }
        Assert-Elf64 $binaries[0].FullName
        $launcher = Get-Content -LiteralPath $launchers[0].FullName -Raw
        if (-not $launcher.StartsWith('#!') -or $launcher.Contains("`r")) { throw 'Linux launcher must have a shebang and LF line endings.' }
    }
    [pscustomobject]@{ Directory = $directory; Version = $version }
}

function Get-ItchTarget([ValidateSet('Windows', 'Linux')][string]$Platform) {
    if ($Platform -eq 'Linux') { return 'aaggeeaa/respawn-and-rage:linux-x86_64' }
    return 'aaggeeaa/respawn-and-rage:windows-x86_64'
}

function Invoke-ItchUpload([string]$ArchivePath, [string]$ExpectedVersion,
        [ValidateSet('Windows', 'Linux')][string]$Platform = 'Windows') {
    $package = Get-GamePackage $ArchivePath $ExpectedVersion $Platform
    $itchTarget = Get-ItchTarget $Platform
    $uploadReceiptFile = Join-Path (Split-Path $ArchivePath -Parent) 'itch-upload.json'
    $uploadReceipt = [ordered]@{
        status = 'uploading'; target = $itchTarget; version = $package.Version
        directory = $package.Directory; started = (Get-Date).ToString('o')
    }
    try {
        Write-Host "Uploading $($package.Version) to $itchTarget"
        $pushArguments = @(Get-ButlerArguments) + @('push', $package.Directory, $itchTarget, '--userversion', $package.Version)
        if ($Platform -eq 'Linux') { $pushArguments += '--fix-permissions' }
        Invoke-Native $Butler $pushArguments
        $uploadReceipt.status = 'uploaded'
        Write-Host "ITCH_UPLOAD_PASS: $itchTarget ($($package.Version))" -ForegroundColor Green
    } catch {
        $uploadReceipt.status = 'failed'
        $uploadReceipt['error'] = $_.Exception.Message
        throw
    } finally {
        $uploadReceipt['finished'] = (Get-Date).ToString('o')
        $uploadReceipt | ConvertTo-Json | Set-Content -LiteralPath $uploadReceiptFile -Encoding UTF8
    }
}

function Invoke-AttemptUpload([string]$Attempt, [switch]$ValidateOnly) {
    $receiptFile = Join-Path $Attempt 'receipt.json'
    $receipt = Get-Content -LiteralPath $receiptFile -Raw | ConvertFrom-Json
    $platform = $receipt.platform
    if (-not $platform) { $platform = 'Windows' } # Legacy Windows-only receipts.
    if ($platform -notin 'Windows', 'Linux' -or $receipt.status -notin 'packaged', 'upload_failed', 'uploaded' -or -not $receipt.game_version) {
        throw "Not a completed Windows/Linux package: $Attempt"
    }
    $archive = Join-Path $Attempt 'dist'
    $null = Get-GamePackage $archive $receipt.game_version $platform
    if ($ValidateOnly) { return }
    $script:logFile = Join-Path $Attempt 'upload.log'
    $receipt | Add-Member -NotePropertyName itch_target -NotePropertyValue (Get-ItchTarget $platform) -Force
    try {
        Invoke-ItchUpload $archive $receipt.game_version $platform
        $receipt.status = 'uploaded'
        $receipt.PSObject.Properties.Remove('error')
    } catch {
        $receipt.status = 'upload_failed'
        $receipt | Add-Member -NotePropertyName error -NotePropertyValue $_.Exception.Message -Force
        throw
    } finally {
        $receipt.finished = (Get-Date).ToString('o')
        $receipt | ConvertTo-Json | Set-Content -LiteralPath $receiptFile -Encoding UTF8
    }
}

try {
    if ($env:OS -ne 'Windows_NT' -or -not [Environment]::Is64BitProcess) {
        throw 'Run this script in 64-bit PowerShell on Windows (5.1 or newer).'
    }
    # Preserve caller-relative tool/identity paths when the build changes cwd.
    if ($Butler -match '[/\\]') { $Butler = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Butler) }
    if ($ButlerIdentity) { $ButlerIdentity = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ButlerIdentity) }
    if ($Check -and $InitAssets) { throw 'Use -Check or -InitAssets separately; -Check does not change asset checkouts.' }
    if ($UploadOnly) {
        if ($NoUpload -or $InitAssets) { throw '-UploadOnly cannot be combined with -NoUpload or -InitAssets.' }
        $runDirectory = (Resolve-Path -LiteralPath $UploadOnly).ProviderPath
        $attempts = @()
        if (Test-Path -LiteralPath (Join-Path $runDirectory 'receipt.json')) { $attempts = @($runDirectory) }
        else {
            foreach ($platform in $platforms) {
                $candidate = Join-Path $runDirectory $platform.ToLowerInvariant()
                $receiptPath = Join-Path $candidate 'receipt.json'
                if (Test-Path -LiteralPath $receiptPath) {
                    $state = Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json
                    if ($state.status -in 'packaged', 'upload_failed') { $attempts += $candidate }
                }
            }
        }
        if (-not $attempts.Count) { throw 'No pending packaged builds found. Pass a platform attempt directory to re-upload an already uploaded build.' }
        foreach ($attempt in $attempts) { Invoke-AttemptUpload $attempt -ValidateOnly }
        Assert-Butler
        if ($Check) { Write-Host 'DESKTOP_UPLOAD_PREFLIGHT_PASS'; exit 0 }
        $failed = $false
        foreach ($attempt in $attempts) {
            try { Invoke-AttemptUpload $attempt } catch { Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red; $failed = $true }
        }
        if ($failed) { exit 1 }
        exit 0
    }
    if (-not (Test-Path -LiteralPath $uproject)) { throw "Project missing: $uproject" }
    if (-not $OutputRoot) { $OutputRoot = Join-Path (Split-Path $projectRoot -Parent) 'builds\desktop' }
    $OutputRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputRoot)
    if ($OutputRoot -eq $projectRoot -or $OutputRoot.StartsWith($projectRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Choose -OutputRoot outside the game repository.'
    }
    Write-Host "Project: $projectRoot"
    Write-Host "Platforms: $($platforms -join ', ') | Configuration: $Configuration | Output: $OutputRoot"
    if (-not $NoUpload) {
        Write-Host 'Upload after packaging: aaggeeaa/respawn-and-rage (one channel per platform)'
        Test-Dependency 'Butler and itch.io authentication' { Assert-Butler }
    }

    Test-Dependency 'Git and Git LFS' {
        Invoke-Native 'git.exe' @('--version')
        Invoke-Native 'git.exe' @('lfs', 'version')
        Invoke-Native 'git.exe' @('-C', $projectRoot, 'rev-parse', '--verify', 'HEAD')
    }
    Test-Dependency 'Python 3.12+ on PATH as python' {
        # Target.cs also calls python, so a working py.exe alone is insufficient.
        Invoke-Native 'python.exe' @('-c', 'import sys; print(sys.version); sys.exit(0 if sys.version_info >= (3,12) else 1)')
    }
    Test-Dependency 'Unreal Engine 5.8.3 Installed Build' {
        $script:resolvedEngine = Find-Engine
        $version = Get-Content -LiteralPath (Join-Path $resolvedEngine 'Engine\Build\Build.version') -Raw | ConvertFrom-Json
        if ("$($version.MajorVersion).$($version.MinorVersion).$($version.PatchVersion)" -ne '5.8.3') {
            throw 'This project requires exactly UE 5.8.3.'
        }
        foreach ($file in @('Engine\Build\InstalledBuild.txt', 'Engine\Build\BatchFiles\Build.bat',
                'Engine\Build\BatchFiles\RunUAT.bat', 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe',
                'Engine\Extras\Redist\en-us\UEPrereqSetup_x64.exe',
                'Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll')) {
            if (-not (Test-Path -LiteralPath (Join-Path $resolvedEngine $file))) { throw "Missing $file. Verify the engine in Epic Launcher (Windows support required)." }
        }
        $dotnet = @(Get-ChildItem (Join-Path $resolvedEngine 'Engine\Binaries\ThirdParty\DotNet\*\win-x64\dotnet.exe') -ErrorAction SilentlyContinue)
        if (-not $dotnet.Count) { throw 'Bundled Windows .NET runtime missing. Verify the engine in Epic Launcher.' }
        Invoke-Native $dotnet[0].FullName @('--info')
        Write-Host "Engine: $resolvedEngine"
    }
    Test-Dependency 'MSVC 14.50 x64 and ATL' {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
        if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio 2026 or Build Tools with C++ desktop/game development, MSVC 14.50 x64/x86 and ATL.' }
        $installations = @(& $vswhere -products '*' -version '[18.0,19.0)' -property installationPath)
        if ($LASTEXITCODE -ne 0) { throw 'vswhere failed. Repair Visual Studio Installer.' }
        $toolchains = @(foreach ($installation in $installations) {
            Get-ChildItem (Join-Path $installation 'VC\Tools\MSVC\14.50.*') -Directory -ErrorAction SilentlyContinue
        })
        $usable = @($toolchains | Where-Object {
            (Test-Path -LiteralPath (Join-Path $_.FullName 'bin\Hostx64\x64\cl.exe')) -and
            (Test-Path -LiteralPath (Join-Path $_.FullName 'bin\Hostx64\x64\link.exe')) -and
            (Test-Path -LiteralPath (Join-Path $_.FullName 'atlmfc\include\atlbase.h')) -and
            (Test-Path -LiteralPath (Join-Path $_.FullName 'atlmfc\lib\x64\atls.lib')) -and
            ([version](Get-Item -LiteralPath (Join-Path $_.FullName 'bin\Hostx64\x64\cl.exe')).VersionInfo.ProductVersion -ge [version]'14.50.35723.0')
        })
        if (-not $usable.Count) { throw 'In Visual Studio Installer add/update MSVC 14.50 (14.50.35723+), x64/x86 tools and matching ATL.' }
        Write-Host "Compiler: $($usable[0].FullName)"
    }
    Test-Dependency 'Windows SDK 10.0.22621+' {
        $kits = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots' -ErrorAction SilentlyContinue
        $sdkRoot = $kits.KitsRoot10
        if (-not $sdkRoot) { $sdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10' }
        $versions = @(Get-ChildItem (Join-Path $sdkRoot 'Include') -Directory -ErrorAction SilentlyContinue | Where-Object {
            $_.Name -match '^10\.0\.\d+\.0$' -and [version]$_.Name -ge [version]'10.0.22621.0'
        })
        $usable = @(foreach ($version in $versions) {
            $sdkVersion = $version.Name
            $required = @("Include\$sdkVersion\um\Windows.h", "Include\$sdkVersion\ucrt\stdio.h",
                "Lib\$sdkVersion\um\x64\kernel32.lib", "Lib\$sdkVersion\ucrt\x64\ucrt.lib",
                "bin\$sdkVersion\x64\rc.exe", "bin\$sdkVersion\x64\mt.exe")
            if (-not @($required | Where-Object { -not (Test-Path -LiteralPath (Join-Path $sdkRoot $_)) }).Count) { $sdkVersion }
        })
        if (-not $usable.Count) { throw 'Install a complete Windows SDK 10.0.22621 or newer using Visual Studio Installer.' }
        Write-Host "Windows SDK: $($usable -join ', ')"
    }
    if ($resolvedEngine) {
        if ($platforms -contains 'Linux') {
            Test-Dependency 'Unreal Linux target support' { Assert-LinuxSupport $resolvedEngine }
            Test-Dependency 'Linux v26 toolchain (clang 20.1.8 and C++20 link)' { Assert-LinuxToolchain $resolvedEngine }
        }
        Test-Dependency 'Enabled plugins, including PCGEx 0.79.2' {
            $spec = Get-Content -LiteralPath $uproject -Raw | ConvertFrom-Json
            $enginePlugins = @(Get-ChildItem (Join-Path $resolvedEngine 'Engine\Plugins') -Filter '*.uplugin' -Recurse -File)
            $projectPlugins = @(Get-ChildItem (Join-Path $projectRoot 'Plugins') -Filter '*.uplugin' -Recurse -File -ErrorAction SilentlyContinue)
            foreach ($reference in @($spec.Plugins | Where-Object Enabled)) {
                $matches = @($projectPlugins | Where-Object BaseName -eq $reference.Name)
                if (-not $matches.Count) { $matches = @($enginePlugins | Where-Object BaseName -eq $reference.Name) }
                if ($matches.Count -ne 1) { throw "Plugin $($reference.Name) missing or ambiguous. Install the UE 5.8 Windows version (engine or project Plugins folder)." }
                $plugin = Get-Content -LiteralPath $matches[0].FullName -Raw | ConvertFrom-Json
                if (($matches[0].FullName -match '[/\\]Marketplace[/\\]' -or $matches[0] -in $projectPlugins) -and $plugin.Modules) {
                    if (-not (Test-Path -LiteralPath (Join-Path $matches[0].DirectoryName 'Source'))) { throw "Plugin source required: $($reference.Name)." }
                }
                if ($platforms -contains 'Linux' -and $plugin.SupportedTargetPlatforms -and 'Linux' -notin $plugin.SupportedTargetPlatforms) {
                    throw "Plugin $($reference.Name) does not support Linux. Install a compatible source version."
                }
                if ($reference.Name -eq 'PCGExtendedToolkit') {
                    if ($plugin.VersionName -ne '0.79.2') { throw "PCGExtendedToolkit 0.79.2 required; found $($plugin.VersionName)." }
                    if (-not (Test-Path -LiteralPath (Join-Path $matches[0].DirectoryName 'Source'))) {
                        throw 'Install PCGExtendedToolkit with Source, needed to compile the Game target.'
                    }
                }
            }
        }
    }
    Test-Dependency 'Unreal Editor closed' {
        if (Get-Process -Name 'UnrealEditor', 'UnrealEditor-Cmd' -ErrorAction SilentlyContinue) { throw 'Close Unreal Editor and other cooks before building or updating assets.' }
    }
    Test-Dependency 'Disk space (10 GiB source / 60 GiB workspace minimum)' {
        Assert-FreeSpace $projectRoot 10
        Assert-FreeSpace $OutputRoot 60
    }
    Test-Dependency 'Runtime branding and font files' {
        foreach ($file in @('Branding\background.jpg', 'Branding\icon.png', 'Branding\Credits.txt',
                'Branding\Fonts\Teko\Fonts\Teko-SemiBold.ttf', 'Branding\Fonts\Teko\ThirdPartyLicenses\Teko-OFL.txt',
                'Branding\Fonts\Teko\CREDITS.txt')) {
            if (-not (Test-Path -LiteralPath (Join-Path $projectRoot $file))) { throw "Missing $file. Restore the complete core checkout." }
        }
    }
    # Avoid fetching/updating submodules until the installed tools pass.
    if ($InitAssets -and $problems.Count -eq 0) {
        Invoke-Native 'python.exe' @((Join-Path $PSScriptRoot 'setup-assets.py'), '--init')
    }
    Test-Dependency 'Private assets and Content mounts' {
        Invoke-Native 'python.exe' @((Join-Path $PSScriptRoot 'setup-assets.py'), '--check')
    }
    if ($problems.Count) {
        throw ("Preflight failed. Fix these items and rerun:`n - " + ($problems -join "`n - ") +
            "`nFor missing assets, authenticate GitHub for all three private repositories and rerun with -InitAssets.")
    }
    Write-Host 'DESKTOP_PREFLIGHT_PASS' -ForegroundColor Green
    if ($Check) { exit 0 }

    $runDirectory = Join-Path $OutputRoot ((Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Path $runDirectory | Out-Null
    $workspace = Join-Path $runDirectory 'project'
    $summaryFile = Join-Path $runDirectory 'summary.json'
    $summary = [ordered]@{
        status = 'running'; configuration = $Configuration; engine = $resolvedEngine; source = $uproject
        started = (Get-Date).ToString('o'); platforms = $platforms; workspace = $workspace
    }
    $logFile = Join-Path $runDirectory 'editor.log'
    Write-Host "Build workspace: $runDirectory"
    try {
        Invoke-Native 'python.exe' @((Join-Path $PSScriptRoot 'prepare-desktop-build.py'), '--engine', $resolvedEngine, '--destination', $workspace)
        $buildProject = Join-Path $workspace 'RespawnAndRage.uproject'
        Push-Location $workspace
        try { Invoke-EditorBuild $resolvedEngine $buildProject } finally { Pop-Location }
        $failed = $false
        foreach ($platform in $platforms) {
            $attempt = Join-Path $runDirectory $platform.ToLowerInvariant()
            New-Item -ItemType Directory -Path $attempt | Out-Null
            $archive = Join-Path $attempt 'dist'
            $logFile = Join-Path $attempt 'build.log'
            $receiptFile = Join-Path $attempt 'receipt.json'
            $receipt = [ordered]@{
                status = 'running'; platform = $platform; configuration = $Configuration; engine = $resolvedEngine
                project = $buildProject; commit = (Get-Content -LiteralPath (Join-Path $workspace 'Build\SourceCommit.txt') -Raw).Trim()
                started = (Get-Date).ToString('o'); archive = $archive; log = $logFile
                itch_target = (Get-ItchTarget $platform)
            }
            try {
                Push-Location $workspace
                try { Invoke-PackageBuild $resolvedEngine $buildProject $archive $Configuration $platform } finally { Pop-Location }
                if ($platform -eq 'Linux') {
                    foreach ($launcher in @(Get-ChildItem -LiteralPath $archive -Recurse -File -Filter '*.sh')) {
                        $text = (Get-Content -LiteralPath $launcher.FullName -Raw).Replace("`r`n", "`n")
                        [IO.File]::WriteAllText($launcher.FullName, $text, (New-Object Text.UTF8Encoding $false))
                    }
                }
                $expectedVersion = (Get-Content -LiteralPath (Join-Path $workspace 'Build\GameVersion.txt') -Raw).Trim()
                $null = Get-GamePackage $archive $expectedVersion $platform
                $receipt.status = 'packaged'
                $receipt['game_version'] = $expectedVersion
                Write-Host "${platform}_PACKAGE_PASS: $archive" -ForegroundColor Green
            } catch {
                $receipt.status = 'failed'
                $receipt['error'] = $_.Exception.Message
                Write-Host "ERROR ($platform): $($_.Exception.Message)" -ForegroundColor Red
                $failed = $true
            } finally {
                $receipt['finished'] = (Get-Date).ToString('o')
                $receipt | ConvertTo-Json | Set-Content -LiteralPath $receiptFile -Encoding UTF8
            }
        }
        if (-not $NoUpload) {
            foreach ($platform in $platforms) {
                $attempt = Join-Path $runDirectory $platform.ToLowerInvariant()
                $state = Get-Content -LiteralPath (Join-Path $attempt 'receipt.json') -Raw | ConvertFrom-Json
                if ($state.status -eq 'packaged') {
                    try { Invoke-AttemptUpload $attempt } catch {
                        Write-Host "ERROR ($platform upload): $($_.Exception.Message). Retry with -UploadOnly '$attempt'." -ForegroundColor Red
                        $failed = $true
                    }
                }
            }
        }
        if ($failed) { throw "One or more builds/uploads failed. Completed packages are retained in $runDirectory." }
        $summary.status = 'uploaded'
        if ($NoUpload) { $summary.status = 'packaged' }
        Write-Host "DESKTOP_BUILD_PASS: $runDirectory" -ForegroundColor Green
    } catch {
        $summary.status = 'failed'
        $summary['error'] = $_.Exception.Message
        throw
    } finally {
        $summary['finished'] = (Get-Date).ToString('o')
        $summary | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $summaryFile -Encoding UTF8
    }
} catch {
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    if ($logFile) { Write-Host "Build log: $logFile" }
    exit 1
} finally {
    $env:LINUX_MULTIARCH_ROOT = $previousLinuxRoot
}
