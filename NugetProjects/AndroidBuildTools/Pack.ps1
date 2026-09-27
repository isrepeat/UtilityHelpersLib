[CmdletBinding()]
param([Parameter(Mandatory)] [string]$FeedPath)

$ErrorActionPreference = 'Stop'
Import-Module -Name (Join-Path $PSScriptRoot 'tools\Modules\Module.AndroidBuildTools\Module.AndroidBuildTools.psm1') -ErrorAction Stop
Module.AndroidBuildTools\Initialize-AndroidBuildConsole
$nuget = (Get-Command nuget.exe -ErrorAction Stop).Source
$manifest = Join-Path $PSScriptRoot 'AndroidBuildTools.nuspec'
$version = ([xml](Get-Content -LiteralPath $manifest -Raw)).package.metadata.version
$output = Join-Path $FeedPath "AndroidBuildTools.$version.nupkg"
if (Test-Path -LiteralPath $output) {
    throw "Package already exists: $output. Increment the package version before publishing."
}
New-Item -ItemType Directory -Path $FeedPath -Force | Out-Null
& $nuget pack $manifest -BasePath $PSScriptRoot -OutputDirectory $FeedPath -NonInteractive -NoPackageAnalysis -NoDefaultExcludes -ForceEnglishOutput
if ($LASTEXITCODE -ne 0) {
    throw "AndroidBuildTools packing failed with exit code $LASTEXITCODE."
}
Write-Host "Package ready: $output"