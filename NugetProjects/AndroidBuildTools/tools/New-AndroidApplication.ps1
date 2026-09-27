[CmdletBinding()]
param(
    [Parameter(Mandatory)] [ValidatePattern('^[A-Z][A-Za-z0-9]*$')] [string]$Name,
    [Parameter(Mandatory)] [ValidatePattern('^[a-z][a-z0-9]*(\.[a-z][a-z0-9]*)+$')] [string]$PackageId,
    [Parameter(Mandatory)] [string]$Destination,
    [Parameter(Mandatory)] [string]$BuildToolsSource,
    [Parameter(Mandatory)] [string]$NativePackageSource,
    [Parameter(Mandatory)] [string]$SecretsRoot,
    [string]$GoogleCloudProject
)

$ErrorActionPreference = 'Stop'
Import-Module -Name (Join-Path $PSScriptRoot 'Modules\Module.AndroidBuildTools\Module.AndroidBuildTools.psm1') -ErrorAction Stop
Module.AndroidBuildTools\Initialize-AndroidBuildConsole
function New-KeyPassword {
    $bytes = [byte[]]::new(20)
    $random = [Security.Cryptography.RandomNumberGenerator]::Create()
    try {
        $random.GetBytes($bytes)
    } finally {
        $random.Dispose()
    }
    return [BitConverter]::ToString($bytes).Replace('-', '').ToLowerInvariant()
}

$destinationRoot = [IO.Path]::GetFullPath($Destination)
if (Test-Path -LiteralPath $destinationRoot) {
    throw "Destination already exists: $destinationRoot. Choose a new directory."
}
$secretsDirectory = Join-Path ([IO.Path]::GetFullPath($SecretsRoot)) $PackageId
if (Test-Path -LiteralPath $secretsDirectory) {
    throw "Secrets directory already exists: $secretsDirectory. Choose another package ID or remove it deliberately."
}
$signingPropertiesPath = Join-Path $secretsDirectory 'signing.properties'

$templateRoot = Join-Path (Split-Path -Parent $PSScriptRoot) 'template_app'
$files = @(Get-ChildItem -LiteralPath $templateRoot -Recurse -File -Force)
[IO.Directory]::CreateDirectory($destinationRoot) | Out-Null
foreach ($file in $files) {
    $relative = $file.FullName.Substring($templateRoot.Length + 1)
    $relative = $relative.Replace('{{Application}}', $Name).Replace('{{PackagePath}}', $PackageId.Replace('.', '/'))
    $target = Join-Path $destinationRoot $relative
    [IO.Directory]::CreateDirectory((Split-Path -Parent $target)) | Out-Null
    if ($file.Extension -eq '.jar') {
        [IO.File]::Copy($file.FullName, $target)
        continue
    }
    $text = [IO.File]::ReadAllText($file.FullName)
    $text = $text.Replace('{{Application}}', $Name).Replace('{{application}}', $Name.ToLowerInvariant()).Replace('{{APPLICATION}}', $Name.ToUpperInvariant())
    $text = $text.Replace('{{PackageId}}', $PackageId).Replace('{{JniPackage}}', $PackageId.Replace('.', '_'))
    $text = $text.Replace('{{BuildToolsSource}}', $BuildToolsSource.Replace("'", "''"))
    $text = $text.Replace('{{NativePackageSource}}', $NativePackageSource.Replace("'", "''"))
    $text = $text.Replace('{{SigningProperties}}', $signingPropertiesPath.Replace("'", "''"))
    [IO.File]::WriteAllText($target, $text.TrimEnd(), [Text.UTF8Encoding]::new($false))
}

[IO.Directory]::CreateDirectory($secretsDirectory) | Out-Null
$debugKeystore = Join-Path $secretsDirectory 'debug.keystore'
$releaseKeystore = Join-Path $secretsDirectory 'release.keystore'
$debugPassword = New-KeyPassword
$releasePassword = New-KeyPassword
$oauthSetup = Join-Path $PSScriptRoot 'Open-AndroidOAuthClientSetup.ps1'
$debugCertificate = & $oauthSetup -ClientName "$Name debug" -PackageId $PackageId -KeystorePath $debugKeystore -KeyAlias debug -StorePassword $debugPassword -KeyPassword $debugPassword -CreateKeystore
$releaseCertificate = & $oauthSetup -ClientName "$Name release" -PackageId $PackageId -KeystorePath $releaseKeystore -KeyAlias release -StorePassword $releasePassword -KeyPassword $releasePassword -CreateKeystore -GoogleCloudProject $GoogleCloudProject -OpenBrowser

$signingProperties = @"
storeFile=$($releaseKeystore.Replace('\', '/'))
storePassword=$releasePassword
keyAlias=release
keyPassword=$releasePassword
debugStoreFile=$($debugKeystore.Replace('\', '/'))
debugStorePassword=$debugPassword
debugKeyAlias=debug
debugKeyPassword=$debugPassword
"@
[IO.File]::WriteAllText($signingPropertiesPath, $signingProperties.TrimEnd(), [Text.UTF8Encoding]::new($false))

$oauthDocument = @'
# Настройка Google OAuth для {{Name}}

Создайте два Android OAuth client в Google Cloud. Сборочный uploader использует
отдельный общий Desktop OAuth client и не использует эти два client ID.

```text
Name: {{Name}} debug
Package name: {{PackageId}}
SHA-1 certificate fingerprint: {{DebugSha1}}

Name: {{Name}} release
Package name: {{PackageId}}
SHA-1 certificate fingerprint: {{ReleaseSha1}}
```

Страница Google Cloud Clients: {{SetupUrl}}

Secrets этого приложения находятся вне Git:

```text
{{SecretsDirectory}}
├─ debug.keystore
├─ release.keystore
└─ signing.properties
```
'@
$setupUrl = if ($releaseCertificate.SetupUrl) { $releaseCertificate.SetupUrl } else { 'передайте -GoogleCloudProject, чтобы получить ссылку на страницу Clients' }
$oauthDocument = $oauthDocument.Replace('{{Name}}', $Name).Replace('{{PackageId}}', $PackageId).Replace('{{DebugSha1}}', $debugCertificate.Sha1).Replace('{{ReleaseSha1}}', $releaseCertificate.Sha1).Replace('{{SecretsDirectory}}', $secretsDirectory).Replace('{{SetupUrl}}', $setupUrl)
[IO.File]::WriteAllText((Join-Path $destinationRoot 'Google-OAuth-setup.md'), $oauthDocument.TrimEnd(), [Text.UTF8Encoding]::new($false))

Write-Host "Application created: $destinationRoot"
Write-Host "Secrets created: $secretsDirectory"
Write-Host 'Run ./build.ps1 build-android -Configuration Debug from that directory.'