[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$packageRoot = Split-Path -Parent $PSScriptRoot
Import-Module -Name (Join-Path $packageRoot 'tools\Modules\Module.AndroidBuildTools\Module.AndroidBuildTools.psm1') -ErrorAction Stop
Module.AndroidBuildTools\Initialize-AndroidBuildConsole
$fixture = Join-Path ([IO.Path]::GetTempPath()) ("AndroidBuildTools-test-" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $fixture | Out-Null
$configuration = @'
@{
    ArtifactName = 'Sample.App'
    AndroidModule = 'Sample.Android'
    AndroidHost = 'Sample.AndroidHost'
    Application = 'Sample.Application'
    UI = 'Sample.UI'
    NativeLibrary = 'libsample.so'
    AndroidPresetPrefix = 'android-arm64'
    CMakeVersionVariable = 'SAMPLE_VERSION'
    GradleRoot = 'Tools/Gradle'
    VersionFile = 'version.properties'
    DistributionDirectory = 'distribution'
    PackageDirectories = @{
        AndroidBuildTools = 'packages'
    }
}
'@
[IO.File]::WriteAllText((Join-Path $fixture 'android-build.psd1'), $configuration)
$versionFile = Join-Path $fixture 'version.properties'
$properties = "VERSION_CODE_BASE=1002000`nVERSION_NAME_BASE=1.2"
[IO.File]::WriteAllText($versionFile, $properties)
$script = Join-Path $packageRoot 'tools/bump-version.ps1'
function Assert-Version {
    param([string]$Name, [int]$Code, [switch]$Keep)
    $actual = & $script -ProjectRoot $fixture -KeepVersion:$Keep
    if ($actual.VERSION_NAME -ne $Name -or $actual.VERSION_CODE -ne $Code) {
        throw "Expected $Name / $Code, got $($actual.VERSION_NAME) / $($actual.VERSION_CODE)."
    }
}
Assert-Version '1.2.1' 1002001
Assert-Version '1.2.0' 1002000 -Keep
$distribution = New-Item -ItemType Directory -Path (Join-Path $fixture 'distribution')
foreach ($name in @('Sample.App-1.2.2.apk', 'Sample.App-1.2.10.apk', 'Sample.App-1.2.9.apk', 'Other-1.2.50.apk', 'SampleXApp-1.2.60.apk', 'Sample.App-2.0.70.apk', 'Sample.App-1002001-1.2.1.apk')) {
    New-Item -ItemType File -Path (Join-Path $distribution.FullName $name) | Out-Null
}
Assert-Version '1.2.11' 1002011
Assert-Version '1.2.10' 1002010 -Keep
foreach ($name in @('Sample.App-1.2.11-debug.apk', 'Sample.App-1.2.12-release.apk')) {
    New-Item -ItemType File -Path (Join-Path $distribution.FullName $name) | Out-Null
}
Assert-Version '1.2.13' 1002013
Assert-Version '1.2.12' 1002012 -Keep
if ([IO.File]::ReadAllText($versionFile) -cne $properties) {
    throw 'Version calculation modified version.properties.'
}
[IO.File]::WriteAllText($versionFile, "VERSION_CODE_BASE=1000000`nVERSION_NAME_BASE=1.2")
$rejected = $false
try {
    & $script -ProjectRoot $fixture | Out-Null
} catch {
    if ($_.Exception.Message -notlike 'VERSION_CODE_BASE must equal*') {
        throw
    }
    $rejected = $true
}
if (-not $rejected) {
    throw 'Mismatched version base was accepted.'
}
Write-Host "Version tests passed. Temporary fixture: $fixture"