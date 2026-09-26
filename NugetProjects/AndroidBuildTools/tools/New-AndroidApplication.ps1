[CmdletBinding()]
param(
    [Parameter(Mandatory)] [ValidatePattern('^[A-Z][A-Za-z0-9]*$')] [string]$Name,
    [Parameter(Mandatory)] [ValidatePattern('^[a-z][a-z0-9]*(\.[a-z][a-z0-9]*)+$')] [string]$PackageId,
    [Parameter(Mandatory)] [string]$Destination,
    [Parameter(Mandatory)] [string]$BuildToolsSource,
    [Parameter(Mandatory)] [string]$NativePackageSource
)

$ErrorActionPreference = 'Stop'
$destinationRoot = [IO.Path]::GetFullPath($Destination)
if (Test-Path -LiteralPath $destinationRoot) {
    throw "Destination already exists: $destinationRoot. Choose a new directory."
}
$templateRoot = Join-Path (Split-Path -Parent $PSScriptRoot) 'templates'
$files = @(Get-ChildItem -LiteralPath $templateRoot -Recurse -File -Force)
[IO.Directory]::CreateDirectory($destinationRoot) | Out-Null
foreach ($file in $files) {
    $relative = $file.FullName.Substring($templateRoot.Length + 1)
    $relative = $relative.Replace('_Application_', $Name).Replace('_PackagePath_', $PackageId.Replace('.', '/'))
    $target = Join-Path $destinationRoot $relative
    [IO.Directory]::CreateDirectory((Split-Path -Parent $target)) | Out-Null
    if ($file.Extension -eq '.jar') {
        [IO.File]::Copy($file.FullName, $target)
        continue
    }
    $text = [IO.File]::ReadAllText($file.FullName)
    $text = $text.Replace('<Application>', $Name).Replace('<application>', $Name.ToLowerInvariant()).Replace('<APPLICATION>', $Name.ToUpperInvariant())
    $text = $text.Replace('<PackageId>', $PackageId).Replace('<JniPackage>', $PackageId.Replace('.', '_'))
    $text = $text.Replace('<BuildToolsSource>', $BuildToolsSource.Replace("'", "''"))
    $text = $text.Replace('<NativePackageSource>', $NativePackageSource.Replace("'", "''"))
    [IO.File]::WriteAllText($target, $text.TrimEnd(), [Text.UTF8Encoding]::new($false))
}
Write-Host "Application created: $destinationRoot"
Write-Host 'Run ./build.ps1 build-android -Configuration Debug from that directory.'