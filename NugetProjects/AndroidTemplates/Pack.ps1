[CmdletBinding()]
param([string]$FeedPath)

$ErrorActionPreference = 'Stop'
$feedResolver = Join-Path $PSScriptRoot '..\..\Scripts\PowerShell\Resolve-PackagesFeed.ps1'
$FeedPath = & $feedResolver -FeedPath $FeedPath
$utf8Encoding = [Text.UTF8Encoding]::new($false)
[Console]::InputEncoding = $utf8Encoding
[Console]::OutputEncoding = $utf8Encoding
$OutputEncoding = $utf8Encoding
$nuget = (Get-Command nuget.exe -ErrorAction Stop).Source
$manifest = Join-Path $PSScriptRoot 'AndroidTemplates.nuspec'

function Get-NextPackageVersion([string]$ManifestPath, [string]$PackagesFeedPath) {
    $manifest = [xml](Get-Content -LiteralPath $ManifestPath -Raw)
    $packageId = $manifest.package.metadata.id
    $baseVersion = $manifest.package.metadata.version
    if ($baseVersion -notmatch '^\d+\.\d+\.\d+$') {
        throw "Package version must use the major.minor.patch format in $ManifestPath."
    }

    $revisions = if (Test-Path -LiteralPath $PackagesFeedPath -PathType Container) {
        Get-ChildItem -LiteralPath $PackagesFeedPath -File -Filter "$packageId.$baseVersion.*.nupkg" | ForEach-Object {
            $versionMatch = [regex]::Match($_.Name, "^$([regex]::Escape($packageId))\.$([regex]::Escape($baseVersion))\.(\d+)\.nupkg$")
            if ($versionMatch.Success) { [int]$versionMatch.Groups[1].Value }
        }
    }
    $maximumRevision = ($revisions | Measure-Object -Maximum).Maximum
    if ($null -eq $maximumRevision) { $maximumRevision = 0 }
    return "$baseVersion.$($maximumRevision + 1)"
}

New-Item -ItemType Directory -Path $FeedPath -Force | Out-Null
$version = Get-NextPackageVersion $manifest $FeedPath
$output = Join-Path $FeedPath "AndroidTemplates.$version.nupkg"
& $nuget pack $manifest -BasePath $PSScriptRoot -OutputDirectory $FeedPath -Version $version -NonInteractive -NoPackageAnalysis -NoDefaultExcludes -ForceEnglishOutput
if ($LASTEXITCODE -ne 0) {
    throw "AndroidTemplates packing failed with exit code $LASTEXITCODE."
}
Write-Host "Package ready: $output"