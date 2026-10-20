#requires -Version 5.1
# Dependency-free helper tests; runs on Windows PowerShell or cross-platform pwsh.
$ErrorActionPreference = 'Stop'
$scriptFile = Join-Path (Split-Path $PSScriptRoot -Parent) 'package-windows.ps1'
$tokens = $null
$parseErrors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile($scriptFile, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw ($parseErrors | Out-String) }
foreach ($definition in $ast.FindAll({ param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] }, $false)) {
    . ([scriptblock]::Create($definition.Extent.Text))
}

function Assert-True($Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
function Assert-Throws([scriptblock]$Action, [string]$Pattern) {
    $message = $null
    try { & $Action } catch { $message = $_.Exception.Message }
    Assert-True ($message -and $message -match $Pattern) "Expected failure matching '$Pattern', received '$message'"
}

$temporary = Join-Path ([IO.Path]::GetTempPath()) ('RR PowerShell tests ' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temporary | Out-Null
$savedProgramFiles = $env:ProgramFiles
$savedProgramData = $env:ProgramData
$savedUserProfile = $env:USERPROFILE
$savedButlerKey = $env:BUTLER_API_KEY
$savedLinuxRoot = $env:LINUX_MULTIARCH_ROOT
$Engine = $null
$logFile = $null
$addedCDrive = $false
try {
    $shell = (Get-Process -Id $PID).Path
    Invoke-Native $shell @('-NoProfile', '-Command', 'exit 0')
    $captured = Invoke-Native $shell @('-NoProfile', '-Command', 'Write-Output "captured compiler version"') -Capture
    Assert-True ($captured -eq 'captured compiler version') 'Native version capture must preserve stdout'
    Assert-Throws { Invoke-Native 'rr-intentionally-missing-executable-041d.exe' @() } 'not recognized|not found'
    Assert-Throws { Invoke-Native $shell @('-NoProfile', '-Command', '[Console]::Error.WriteLine("compiler failure"); exit 23') } 'exit 23'
    # Stderr alone must not fail Windows PowerShell 5.1 when the exit code is 0.
    $logFile = Join-Path $temporary 'build output.log'
    Invoke-Native $shell @('-NoProfile', '-Command', '[Console]::Error.WriteLine("diagnostic on stderr"); Write-Output "normal output"; exit 0')
    $log = Get-Content -LiteralPath $logFile -Raw
    Assert-True ($log -match 'diagnostic on stderr' -and $log -match 'normal output') 'Both native streams must be logged'
    $logFile = $null
    # Arguments containing spaces must survive native invocation unchanged.
    $argumentProbe = Join-Path $temporary 'argument probe.ps1'
    Set-Content -LiteralPath $argumentProbe -Value 'if ($args.Count -ne 1 -or $args[0] -ne "project with spaces") { exit 19 }; exit 0'
    Invoke-Native $shell @('-NoProfile', '-File', $argumentProbe, 'project with spaces')

    $problems = New-Object 'System.Collections.Generic.List[string]'
    Test-Dependency 'missing engine' { throw 'install engine' }
    Test-Dependency 'missing compiler' { throw 'install compiler' }
    Test-Dependency 'working dependency' { }
    Assert-True ($problems.Count -eq 2) 'Preflight must collect independent failures'
    Assert-True ($problems[1] -match 'missing compiler: install compiler') 'Preflight must retain corrective messages'

    # Test actual Launcher JSON discovery with the registry absent. Windows-only
    # locations are mapped into a temporary tree; no real registry is touched.
    function Get-ItemProperty { param($Path, $ErrorAction) return $null }
    if (-not (Get-PSDrive C -ErrorAction SilentlyContinue)) {
        New-PSDrive -Name C -PSProvider FileSystem -Root $temporary | Out-Null
        $addedCDrive = $true
    }
    $env:ProgramFiles = Join-Path $temporary 'Program Files'
    $env:ProgramData = Join-Path $temporary 'ProgramData'
    $preferred = $env:UE_ENGINE_ROOT
    $env:UE_ENGINE_ROOT = $null
    $engineFolder = Join-Path $temporary 'Custom Epic location\UE_5.8'
    $buildFolder = Join-Path $engineFolder 'Engine\Build'
    New-Item -ItemType Directory -Path $buildFolder -Force | Out-Null
    $manifestFolder = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher'
    New-Item -ItemType Directory -Path $manifestFolder -Force | Out-Null
    @{ InstallationList = @(@{ AppName = 'UE_5.8'; InstallLocation = $engineFolder }) } |
        ConvertTo-Json -Depth 4 | Set-Content (Join-Path $manifestFolder 'LauncherInstalled.dat')
    @{ MajorVersion = 5; MinorVersion = 8; PatchVersion = 3 } | ConvertTo-Json |
        Set-Content (Join-Path $buildFolder 'Build.version')
    # On Unix C: is not mounted. Missing Windows default paths are intentionally
    # ignored just as absent directories are on Windows.
    $found = Find-Engine
    Assert-True ($found -eq [IO.Path]::GetFullPath($engineFolder)) 'Discover custom engine from Launcher manifest'
    $Engine = $engineFolder
    Assert-True ((Find-Engine) -eq $found) 'Explicit engine path must override discovery'
    $Engine = $null
    $env:UE_ENGINE_ROOT = $engineFolder
    Assert-True ((Find-Engine) -eq $found) 'Discover engine from UE_ENGINE_ROOT'
    Assert-Throws { Assert-FreeSpace $temporary ([double]::MaxValue) } 'Need at least'
    Assert-FreeSpace (Join-Path $temporary 'not-yet-created\output') 0

    # Mock only external Unreal invocations to verify that an Editor failure
    # never starts UAT, and that a Game failure propagates to the caller.
    $invocations = New-Object 'System.Collections.Generic.List[object]'
    $failOnCall = 1
    function Invoke-Native([string]$FilePath, [string[]]$Arguments, [switch]$Capture) {
        $invocations.Add(@{ path = $FilePath; arguments = $Arguments })
        if ($invocations.Count -eq $failOnCall) { throw 'simulated compiler failure' }
        if ($Capture) { return $fakeCompilerVersion }
        if ($Arguments -contains '--target=x86_64-unknown-linux-gnu') {
            $outputIndex = [Array]::IndexOf($Arguments, '-o') + 1
            [IO.File]::WriteAllBytes($Arguments[$outputIndex], $elf)
        }
    }
    $projectFile = Join-Path $temporary 'Game source\RespawnAndRage.uproject'
    $archiveFolder = Join-Path $temporary 'Output folder\dist'
    Assert-Throws { Invoke-EditorBuild $engineFolder $projectFile } 'compiler failure'
    Assert-True ($invocations.Count -eq 1) 'A failed Editor build must stop before UAT'
    $invocations.Clear()
    $failOnCall = 1
    Assert-Throws { Invoke-PackageBuild $engineFolder $projectFile $archiveFolder 'Shipping' } 'compiler failure'
    Assert-True ($invocations.Count -eq 1) 'A failed UAT invocation must propagate'
    $invocations.Clear()
    $failOnCall = 0
    Invoke-EditorBuild $engineFolder $projectFile
    Invoke-PackageBuild $engineFolder $projectFile $archiveFolder 'Shipping' 'Windows'
    Invoke-PackageBuild $engineFolder $projectFile $archiveFolder 'Shipping' 'Linux'
    Assert-True ($invocations.Count -eq 3) 'Compile Editor once and package both platforms'
    Assert-True ($invocations[0].arguments[2] -eq 'Development') 'Editor configuration must remain Development'
    Assert-True ($invocations[1].arguments -contains '-clientconfig=Shipping') 'Game configuration must honor Shipping'
    Assert-True ($invocations[1].arguments -contains "-archivedirectory=$archiveFolder") 'Archive paths with spaces must remain one argument'
    Assert-True ($invocations[1].arguments -contains '-platform=Win64' -and $invocations[1].arguments -contains '-prereqs') 'Windows build must stage Windows prerequisites'
    Assert-True ($invocations[2].arguments -contains '-platform=Linux' -and $invocations[2].arguments -contains '-clientarchitecture=x64') 'Linux build must target x64'
    Assert-True (-not ($invocations[2].arguments -contains '-prereqs')) 'Linux package must not stage Windows prerequisites'

    $elf = New-Object byte[] 20
    $elf[0] = 127; $elf[1] = 69; $elf[2] = 76; $elf[3] = 70; $elf[4] = 2; $elf[5] = 1; $elf[16] = 3; $elf[18] = 62
    $toolchain = Join-Path $temporary 'Linux toolchain with spaces'
    $archRoot = Join-Path $toolchain 'x86_64-unknown-linux-gnu'
    foreach ($file in @('bin\clang++.exe', 'bin\ld.lld.exe', 'bin\llvm-ar.exe', 'bin\llvm-objcopy.exe',
            'include\c++\v1\vector', 'lib64\libc++.a', 'lib64\libc++abi.a')) {
        $path = Join-Path $archRoot $file
        New-Item -ItemType Directory -Path (Split-Path $path -Parent) -Force | Out-Null
        Set-Content -LiteralPath $path -Value 'fixture'
    }
    $sdkFolder = Join-Path $engineFolder 'Engine\Config\Linux'
    New-Item -ItemType Directory -Path $sdkFolder -Force | Out-Null
    @{ MainVersion = 'v26_clang-20.1.8-rockylinux8' } | ConvertTo-Json | Set-Content (Join-Path $sdkFolder 'Linux_SDK.json')
    $LinuxToolchain = $toolchain
    $marker = Join-Path $toolchain 'ToolchainVersion.txt'
    Set-Content -LiteralPath $marker -Value 'v25_clang-18.1.0-rockylinux8'
    Assert-Throws { Assert-LinuxToolchain $engineFolder } 'Install Epic v26'
    Set-Content -LiteralPath $marker -Value 'v26_clang-20.1.8-rockylinux8'
    $fakeCompilerVersion = 'clang version 19.0.0'
    Assert-Throws { Assert-LinuxToolchain $engineFolder } 'must run clang 20.1.8'
    $fakeCompilerVersion = 'clang version 20.1.8 (Epic)'
    $invocations.Clear()
    Assert-LinuxToolchain $engineFolder
    Assert-True ($env:LINUX_MULTIARCH_ROOT -eq $toolchain) 'Validated toolchain must be the one passed to UBT'
    Assert-True ($invocations.Count -eq 2) 'Toolchain preflight must run compiler version and C++ link'
    $invocations.Clear()
    $failOnCall = 2
    Assert-Throws { Assert-LinuxToolchain $engineFolder } 'compiler failure'
    $failOnCall = 0
    Remove-Item -LiteralPath (Join-Path $archRoot 'lib64\libc++abi.a')
    Assert-Throws { Assert-LinuxToolchain $engineFolder } 'Incomplete Linux toolchain'
    $Configuration = 'Development'
    $engineWin64 = Join-Path $engineFolder 'Engine\Binaries\Win64'
    $engineLinux = Join-Path $engineFolder 'Engine\Binaries\Linux'
    New-Item -ItemType Directory -Path $engineWin64, $engineLinux -Force | Out-Null
    Assert-Throws { Assert-LinuxSupport $engineFolder } 'enable Linux target support'
    Set-Content -LiteralPath (Join-Path $engineWin64 'UnrealEditor-LinuxTargetPlatform.dll') -Value 'fixture'
    Set-Content -LiteralPath (Join-Path $engineLinux 'UnrealGame.target') -Value 'fixture'
    Assert-LinuxSupport $engineFolder
    $Configuration = 'Shipping'
    Assert-Throws { Assert-LinuxSupport $engineFolder } 'enable Linux target support'
    Set-Content -LiteralPath (Join-Path $engineLinux 'UnrealGame-Linux-Shipping.target') -Value 'fixture'
    Assert-LinuxSupport $engineFolder

    $Butler = 'mock-butler.exe'
    $ButlerIdentity = $null
    $itchTarget = 'aaggeeaa/respawn-and-rage:windows-x86_64'
    $env:BUTLER_API_KEY = $null
    $env:USERPROFILE = $temporary
    $invocations.Clear()
    Assert-Throws { Assert-Butler } 'Run butler login first'
    Assert-True ($invocations.Count -eq 1) 'Missing credentials must stop before an API call or push'
    $env:BUTLER_API_KEY = 'test-only-not-a-credential'
    $invocations.Clear()
    Assert-Butler
    Assert-True ($invocations.Count -eq 2 -and $invocations[1].arguments[0] -eq 'status') 'Auth preflight must use read-only status'
    Assert-True (-not ($invocations[1].arguments -contains $env:BUTLER_API_KEY)) 'API key must never enter command arguments'
    $invocations.Clear()
    $failOnCall = 2
    Assert-Throws { Assert-Butler } 'could not access'
    Assert-True ($invocations.Count -eq 2) 'Failed auth preflight must not push'
    $failOnCall = 0
    $env:BUTLER_API_KEY = $null
    $ButlerIdentity = Join-Path $temporary 'private identity file'
    Set-Content -LiteralPath $ButlerIdentity -Value 'test-only-not-a-credential'
    $invocations.Clear()
    Assert-Butler
    Assert-True ($invocations[1].arguments[0] -eq '--identity' -and $invocations[1].arguments[1] -eq $ButlerIdentity) 'Explicit identity path must survive spaces'

    $staged = Join-Path $archiveFolder 'Windows'
    $paks = Join-Path $staged 'RespawnAndRage\Content\Paks'
    $stampFolder = Join-Path $staged 'RespawnAndRage\Build'
    $runtimeFolder = Join-Path $staged 'RespawnAndRage\Binaries\Win64'
    New-Item -ItemType Directory -Path $paks, $stampFolder, $runtimeFolder -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $staged 'RespawnAndRage.exe') -Value 'fixture'
    Set-Content -LiteralPath (Join-Path $runtimeFolder 'RespawnAndRage.exe') -Value 'fixture'
    Set-Content -LiteralPath (Join-Path $paks 'RespawnAndRage-Windows.utoc') -Value 'fixture'
    $stampFile = Join-Path $stampFolder 'GameVersion.txt'
    Set-Content -LiteralPath $stampFile -Value '26.10.20.abcdef12'
    $invocations.Clear()
    Invoke-ItchUpload $archiveFolder '26.10.20.abcdef12'
    Assert-True ($invocations.Count -eq 1) 'Upload must run exactly one push'
    $pushed = $invocations[0].arguments
    Assert-True ($pushed -contains $staged) 'Push only the staged game root, excluding sibling logs and receipts'
    Assert-True (-not ($pushed -contains $archiveFolder)) 'Do not push the parent archive directory'
    Assert-True ($pushed -contains $itchTarget -and $pushed[-1] -eq '26.10.20.abcdef12') 'Push Windows channel with packaged version'
    $uploadReceiptFile = Join-Path (Split-Path $archiveFolder -Parent) 'itch-upload.json'
    $uploadReceipt = Get-Content -LiteralPath $uploadReceiptFile -Raw | ConvertFrom-Json
    Assert-True ($uploadReceipt.status -eq 'uploaded') 'Successful push must persist upload status'
    $invocations.Clear()
    $failOnCall = 1
    Assert-Throws { Invoke-ItchUpload $archiveFolder '26.10.20.abcdef12' } 'compiler failure'
    $uploadReceipt = Get-Content -LiteralPath $uploadReceiptFile -Raw | ConvertFrom-Json
    Assert-True ($uploadReceipt.status -eq 'failed' -and (Test-Path -LiteralPath $stampFile)) 'Upload failure must be recorded without removing the package'
    $failOnCall = 0
    $invocations.Clear()
    Assert-Throws { Invoke-ItchUpload $archiveFolder '26.10.19.abcdef12' } 'version differs'
    Set-Content -LiteralPath $stampFile -Value 'invalid version'
    Assert-Throws { Invoke-ItchUpload $archiveFolder $null } 'Invalid packaged game version'
    Assert-True ($invocations.Count -eq 0) 'Reject invalid packages before invoking Butler'
    Set-Content -LiteralPath $stampFile -Value '26.10.20.abcdef12'
    Remove-Item -LiteralPath (Join-Path $paks 'RespawnAndRage-Windows.utoc')
    Assert-Throws { Invoke-ItchUpload $archiveFolder $null } 'containers are missing'

    $linuxArchive = Join-Path $temporary 'linux attempt\dist'
    $linuxStage = Join-Path $linuxArchive 'Linux'
    $linuxGame = Join-Path $linuxStage 'RespawnAndRage'
    foreach ($folder in @('Content\Paks', 'Build', 'Binaries\Linux')) { New-Item -ItemType Directory -Path (Join-Path $linuxGame $folder) -Force | Out-Null }
    [IO.File]::WriteAllText((Join-Path $linuxStage 'RespawnAndRage.sh'), "#!/bin/sh`nexit 0`n")
    Set-Content -LiteralPath (Join-Path $linuxGame 'Build\GameVersion.txt') -Value '26.10.20.abcdef12'
    Set-Content -LiteralPath (Join-Path $linuxGame 'Content\Paks\game.pak') -Value 'fixture'
    $linuxBinary = Join-Path $linuxGame 'Binaries\Linux\RespawnAndRage-Linux-Shipping'
    [IO.File]::WriteAllBytes($linuxBinary, $elf)
    $invocations.Clear()
    Invoke-ItchUpload $linuxArchive '26.10.20.abcdef12' 'Linux'
    Assert-True ($invocations[0].arguments -contains 'aaggeeaa/respawn-and-rage:linux-x86_64') 'Linux must upload to its own channel'
    Assert-True ($invocations[0].arguments -contains '--fix-permissions') 'Linux upload must restore executable permissions on Windows'
    Assert-True ($invocations[0].arguments -contains $linuxStage) 'Linux upload must use the staged root'
    $elf[18] = 183
    [IO.File]::WriteAllBytes($linuxBinary, $elf)
    Assert-Throws { Invoke-ItchUpload $linuxArchive $null 'Linux' } 'Linux x86_64 ELF'
    $elf[18] = 62
    [IO.File]::WriteAllBytes($linuxBinary, $elf)
    [IO.File]::WriteAllText((Join-Path $linuxStage 'RespawnAndRage.sh'), "#!/bin/sh`r`nexit 0`r`n")
    Assert-Throws { Invoke-ItchUpload $linuxArchive $null 'Linux' } 'LF line endings'
    Write-Host 'WINDOWS_HELPER_TESTS_PASS'
} finally {
    $env:ProgramFiles = $savedProgramFiles
    $env:ProgramData = $savedProgramData
    $env:USERPROFILE = $savedUserProfile
    $env:BUTLER_API_KEY = $savedButlerKey
    $env:LINUX_MULTIARCH_ROOT = $savedLinuxRoot
    if (Test-Path variable:preferred) { $env:UE_ENGINE_ROOT = $preferred }
    if ($addedCDrive) { Remove-PSDrive C }
    Remove-Item -LiteralPath $temporary -Recurse -Force
}
