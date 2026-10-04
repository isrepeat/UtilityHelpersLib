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
    # Полная версия хранится в исходниках; увеличиваем только последнюю часть.
    $content = [System.IO.File]::ReadAllText($ManifestPath)
    $match = [regex]::Match($content, '<version>(\d+(?:\.\d+){2,3})</version>')
    if (-not $match.Success) { throw "Full package version is missing in $ManifestPath." }
    $parts = $match.Groups[1].Value.Split('.')
    $parts[$parts.Length - 1] = ([int]$parts[$parts.Length - 1] + 1).ToString()
    $nextVersion = $parts -join '.'
    $content = $content.Remove($match.Groups[1].Index, $match.Groups[1].Length).Insert($match.Groups[1].Index, $nextVersion)
    [System.IO.File]::WriteAllText($ManifestPath, $content.TrimEnd(), [System.Text.UTF8Encoding]::new($false))
    return $nextVersion
}

New-Item -ItemType Directory -Path $FeedPath -Force | Out-Null
$version = Get-NextPackageVersion $manifest $FeedPath
$output = Join-Path $FeedPath "AndroidTemplates.$version.nupkg"
& $nuget pack $manifest -BasePath $PSScriptRoot -OutputDirectory $FeedPath -Version $version -NonInteractive -NoPackageAnalysis -NoDefaultExcludes -ForceEnglishOutput
if ($LASTEXITCODE -ne 0) {
    throw "AndroidTemplates packing failed with exit code $LASTEXITCODE."
}
Write-Host "Package ready: $output"