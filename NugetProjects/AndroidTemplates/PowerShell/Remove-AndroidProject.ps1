[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$ProjectRoot,

    [switch]$Confirmed,

    [string]$SigningProperties
)

$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath($ProjectRoot)
$configurationPath = Join-Path $projectRoot 'android-build.psd1'
if (-not (Test-Path -LiteralPath $configurationPath -PathType Leaf)) {
    throw "Project configuration was not found: $configurationPath"
}

$configuration = Import-PowerShellDataFile -LiteralPath $configurationPath
$signingPropertiesPath = if ($SigningProperties) { $SigningProperties } else { $configuration.SigningProperties }
if (-not $signingPropertiesPath) {
    . (Join-Path $PSScriptRoot 'Read-AndroidSharedProps.ps1')
    $sharedProps = Read-AndroidSharedProps -ProjectRoot $projectRoot
    $signingPropertiesPath = $sharedProps.Paths.SigningProperties
}

$secretsDirectory = Split-Path -Parent $signingPropertiesPath
$sharedKeys = (Split-Path -Leaf $secretsDirectory) -eq 'shared'
$packageId = Split-Path -Leaf $projectRoot
if (-not $Confirmed) {
    $description = if ($sharedKeys) { "Delete project $packageId (shared signing keys are preserved)? [Y/N]" } else { "Delete project and secrets for $packageId? [Y/N]" }
    $answer = Read-Host $description
    if ($answer -ine 'Y') {
        Write-Host 'Deletion cancelled.'
        return
    }
}

$normalizedProjectRoot = $projectRoot.TrimEnd('\') + '\'
$scriptPath = [IO.Path]::GetFullPath($PSCommandPath)
if ($scriptPath.StartsWith($normalizedProjectRoot, [StringComparison]::OrdinalIgnoreCase)) {
    $temporaryScript = Join-Path ([IO.Path]::GetTempPath()) ("Remove-AndroidProject-{0}.ps1" -f [Guid]::NewGuid().ToString('N'))
    Copy-Item -LiteralPath $scriptPath -Destination $temporaryScript -Force
    try {
        & $temporaryScript -ProjectRoot $projectRoot -Confirmed -SigningProperties $signingPropertiesPath
        exit $LASTEXITCODE
    } finally {
        Remove-Item -LiteralPath $temporaryScript -Force -ErrorAction SilentlyContinue
    }
}

Set-Location -LiteralPath ([IO.Path]::GetTempPath())
if (-not $sharedKeys -and (Test-Path -LiteralPath $secretsDirectory -PathType Container)) {
    Write-Host "Removing secrets: $secretsDirectory"
    Remove-Item -LiteralPath $secretsDirectory -Recurse -Force
}
Write-Host "Removing project: $projectRoot"
Remove-Item -LiteralPath $projectRoot -Recurse -Force