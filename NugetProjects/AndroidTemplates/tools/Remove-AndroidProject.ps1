[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$ProjectRoot,

    [switch]$Confirmed
)

$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath($ProjectRoot)
$configurationPath = Join-Path $projectRoot 'android-build.psd1'
if (-not (Test-Path -LiteralPath $configurationPath -PathType Leaf)) {
    throw "Project configuration was not found: $configurationPath"
}

$configuration = Import-PowerShellDataFile -LiteralPath $configurationPath
$signingPropertiesPath = $configuration.SigningProperties
if (-not $signingPropertiesPath) {
    throw 'SigningProperties is required to locate this project secrets directory.'
}

$secretsDirectory = Split-Path -Parent $signingPropertiesPath
if (-not (Test-Path -LiteralPath $secretsDirectory -PathType Container)) {
    throw "Secrets directory was not found: $secretsDirectory"
}
$packageId = Split-Path -Leaf $secretsDirectory

if (-not $Confirmed) {
    $answer = Read-Host "Delete project and secrets for $PackageId? [Y/N]"
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
        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $temporaryScript -ProjectRoot $projectRoot -Confirmed
        exit $LASTEXITCODE
    } finally {
        Remove-Item -LiteralPath $temporaryScript -Force -ErrorAction SilentlyContinue
    }
}

Set-Location -LiteralPath ([IO.Path]::GetTempPath())
Write-Host "Removing secrets: $secretsDirectory"
Remove-Item -LiteralPath $secretsDirectory -Recurse -Force
Write-Host "Removing project: $projectRoot"
Remove-Item -LiteralPath $projectRoot -Recurse -Force