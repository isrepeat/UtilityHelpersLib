[CmdletBinding()]
param([string]$FeedPath = $env:UH_PACKAGES_FEED)

$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($FeedPath)) {
    $FeedPath = Read-Host 'Packages feed path'
}

if ([string]::IsNullOrWhiteSpace($FeedPath)) {
    throw 'Packages feed path is required. Set UH_PACKAGES_FEED or enter the path.'
}

[IO.Path]::GetFullPath($FeedPath)