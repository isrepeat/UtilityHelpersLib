[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$ProjectRoot,

    [Parameter(Position = 0)]
    [ValidateSet('restore', 'build-android', 'build-and-distribute', 'build-for-drive', 'build-all', 'bump-version', 'generate-xaml', 'upload-apk-to-drive', 'run-android-app-previewer')]
    [string]$Command = 'build-android',

    [ValidateSet('Debug', 'Release')]
    [string]$Configuration,
    [ValidateSet('arm64-v8a')]
    [string]$Architecture,
    [ValidateSet('Local', 'Drive')]
    [string]$Destination,
    [switch]$Clean,
    [switch]$NativeOnly,
    [switch]$KeepVersion,
    [int]$AppVersionCode,
    [string]$AppVersionName,
    [string]$CMakeExecutable,
    [string]$ApkPath,
    [string]$OAuthClientPath,
    [string]$TokenPath,
    [string[]]$DrivePath,
    [string]$DriveFileName,
    [int]$ParentProcessId,
    [switch]$BuildOnly
)

$ErrorActionPreference = 'Stop'
$config = Import-PowerShellDataFile (Join-Path $ProjectRoot 'android-build.psd1')
$packageRoot = Split-Path -Parent $PSScriptRoot

$parameters = @{}
foreach ($name in $PSBoundParameters.Keys) {
    if ($name -notin @('Command', 'ProjectRoot')) {
        $parameters[$name] = $PSBoundParameters[$name]
    }
}
if ($Command -eq 'restore') {
    if ($parameters.Count -gt 0) {
        throw 'The restore command does not accept build parameters.'
    }
    return $packageRoot
}
if ($Command -in @('build-for-drive', 'build-all')) {
    $Command = 'build-and-distribute'
    $parameters.Destination = 'Drive'
}
$script = Join-Path $packageRoot "tools\$Command.ps1"
$metadata = Get-Command -Name $script -ErrorAction Stop
foreach ($name in $parameters.Keys) {
    if (-not $metadata.Parameters.ContainsKey($name)) {
        throw "The $Command command does not accept -$name."
    }
}
if ($Command -eq 'upload-apk-to-drive') {
    foreach ($name in @('OAuthClientPath', 'TokenPath')) {
        if (-not $parameters.ContainsKey($name)) {
            $parameters[$name] = $config.Drive[$name]
        }
    }
    if (-not $parameters.ContainsKey('DrivePath')) {
        $parameters.DrivePath = $config.Drive.Path
    }
} else {
    $parameters.ProjectRoot = $ProjectRoot
}
& $script @parameters