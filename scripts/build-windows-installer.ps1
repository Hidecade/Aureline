[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string] $Configuration = "Release",
    [string] $BuildDirectory = "build/windows-release",
    [string] $JuceDirectory = "",
    [string] $InnoSetupCompiler = "",
    [switch] $SkipBuild
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent $PSScriptRoot
$cmakeFile = Join-Path $repoRoot "CMakeLists.txt"
$cmakeText = Get-Content -Raw $cmakeFile
if ($cmakeText -notmatch 'project\(Aureline VERSION ([0-9]+\.[0-9]+\.[0-9]+)') {
    throw "Could not read the Aureline version from CMakeLists.txt"
}
$version = $Matches[1]

if (-not [IO.Path]::IsPathRooted($BuildDirectory)) {
    $BuildDirectory = Join-Path $repoRoot $BuildDirectory
}

if (-not $SkipBuild) {
    $configureArguments = @(
        "-S", $repoRoot,
        "-B", $BuildDirectory,
        "-A", "x64",
        "-DAURELINE_BUILD_STANDALONE=ON",
        "-DAURELINE_BUILD_PLUGINS=ON"
    )
    if ($JuceDirectory) {
        $configureArguments += "-DAURELINE_JUCE_DIR=$JuceDirectory"
    }

    & cmake @configureArguments
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

    & cmake --build $BuildDirectory --config $Configuration --target Aureline_Plugin_Standalone Aureline_Plugin_VST3 aureline_engine_tests
    if ($LASTEXITCODE -ne 0) { throw "CMake build failed" }

    & ctest --test-dir $BuildDirectory -C $Configuration --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "Tests failed" }
}

$artifactRoot = Join-Path $BuildDirectory "Aureline_Plugin_artefacts/$Configuration"
$standalone = Join-Path $artifactRoot "Standalone/Aureline.exe"
$vst3 = Join-Path $artifactRoot "VST3/Aureline.vst3"
foreach ($requiredArtifact in @($standalone, $vst3)) {
    if (-not (Test-Path $requiredArtifact)) {
        throw "Required artifact was not found: $requiredArtifact"
    }
}

if (-not $InnoSetupCompiler) {
    $compilerCandidates = @(
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
        "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
        "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
    )
    $InnoSetupCompiler = $compilerCandidates |
        Where-Object { $_ -and (Test-Path $_) } |
        Select-Object -First 1
}
if (-not $InnoSetupCompiler -or -not (Test-Path $InnoSetupCompiler)) {
    throw "Inno Setup 6 was not found. Install it or pass -InnoSetupCompiler."
}

$distDirectory = Join-Path $repoRoot "dist"
New-Item -ItemType Directory -Force $distDirectory | Out-Null
$issFile = Join-Path $repoRoot "scripts/windows/Aureline.iss"

& $InnoSetupCompiler `
    "/DMyAppVersion=$version" `
    "/DStandaloneSource=$standalone" `
    "/DVst3Source=$vst3" `
    "/DOutputDirectory=$distDirectory" `
    $issFile
if ($LASTEXITCODE -ne 0) { throw "Inno Setup compilation failed" }

$installer = Join-Path $distDirectory "Aureline-$version-Windows-x64-Setup.exe"
if (-not (Test-Path $installer)) {
    throw "Installer was not created: $installer"
}

$hash = (Get-FileHash -Algorithm SHA256 $installer).Hash.ToLowerInvariant()
$checksumFile = "$installer.sha256"
"$hash  $(Split-Path -Leaf $installer)" | Set-Content -Encoding ascii $checksumFile

Write-Host "Windows installer created:"
Write-Host "  $installer"
Write-Host "  $checksumFile"
