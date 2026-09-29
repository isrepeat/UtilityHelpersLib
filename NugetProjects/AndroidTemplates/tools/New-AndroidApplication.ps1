[CmdletBinding()]
param(
    [Parameter(Mandatory)] [ValidatePattern('^[A-Z][A-Za-z0-9]*$')] [string]$Name,
    [Parameter(Mandatory)] [ValidatePattern('^[a-z][a-z0-9]*(\.[a-z][a-z0-9]*)+$')] [string]$PackageId,
    [Parameter(Mandatory)] [string]$Destination,
    [Parameter(Mandatory)] [string]$BuildToolsSource,
    [Parameter(Mandatory)] [string]$NativePackageSource,
    [Parameter(Mandatory)] [string]$SecretsRoot,
    [Parameter(Mandatory)] [string]$DriveOAuthClientPath,
    [Parameter(Mandatory)] [string]$DriveTokenPath,
    [Parameter(Mandatory)] [string]$PreviewerDebugExecutablePath,
    [Parameter(Mandatory)] [string]$PreviewerReleaseExecutablePath,
    [string]$GoogleCloudProject
)

$ErrorActionPreference = 'Stop'
$utf8Encoding = [Text.UTF8Encoding]::new($false)
[Console]::InputEncoding = $utf8Encoding
[Console]::OutputEncoding = $utf8Encoding
$OutputEncoding = $utf8Encoding
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

function Resolve-LatestAndroidBuildToolsVersion {
    param([string]$Source)

    $nuget = (Get-Command nuget.exe -ErrorAction Stop).Source
    $output = & $nuget list AndroidBuildTools -Source $Source -AllVersions -Prerelease -NonInteractive -ForceEnglishOutput 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "AndroidBuildTools version lookup failed for ${Source} with exit code ${LASTEXITCODE}: $($output -join [Environment]::NewLine)"
    }

    $versions = @(
        $output | ForEach-Object {
            $match = [regex]::Match($_.ToString(), '^\s*AndroidBuildTools\s+(?<Version>\d+(?:\.\d+){2,3})(?:\s|$)')
            if ($match.Success) {
                [version]$match.Groups['Version'].Value
            }
        }
    )
    $latestVersion = $versions | Sort-Object -Descending | Select-Object -First 1
    if ($null -eq $latestVersion) {
        throw "AndroidBuildTools was not found in $Source. Publish it to the feed before creating a project."
    }

    return $latestVersion
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
$androidBuildToolsVersion = (Resolve-LatestAndroidBuildToolsVersion $BuildToolsSource).ToString()
$versionPropertiesPath = Join-Path $templateRoot 'version.properties'
$versionProperties = ConvertFrom-StringData ([IO.File]::ReadAllText($versionPropertiesPath))
$applicationVersion = $versionProperties.VERSION_NAME_BASE
if ([string]::IsNullOrWhiteSpace($applicationVersion)) {
    throw 'version.properties must define VERSION_NAME_BASE.'
}
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
    $text = $text.Replace('{{Application}}', $Name).Replace('{{application}}', $Name.ToLowerInvariant()).Replace('{{APPLICATION}}', $Name.ToUpperInvariant()).Replace('{{ApplicationVersion}}', $applicationVersion).Replace('{{AndroidBuildToolsVersion}}', $androidBuildToolsVersion)
    $text = $text.Replace('{{PackageId}}', $PackageId).Replace('{{JniPackage}}', $PackageId.Replace('.', '_'))
    $text = $text.Replace('{{BuildToolsSource}}', $BuildToolsSource.Replace("'", "''"))
    $text = $text.Replace('{{NativePackageSource}}', $NativePackageSource.Replace("'", "''"))
    $text = $text.Replace('{{SigningProperties}}', $signingPropertiesPath.Replace("'", "''"))
    $text = $text.Replace('{{DriveOAuthClientPath}}', $DriveOAuthClientPath.Replace("'", "''"))
    $text = $text.Replace('{{DriveTokenPath}}', $DriveTokenPath.Replace("'", "''"))
    $text = $text.Replace('{{PreviewerDebugExecutablePath}}', $PreviewerDebugExecutablePath.Replace("'", "''"))
    $text = $text.Replace('{{PreviewerReleaseExecutablePath}}', $PreviewerReleaseExecutablePath.Replace("'", "''"))
    $encoding = if ($file.Extension -eq '.ps1') { [Text.UTF8Encoding]::new($true) } else { [Text.UTF8Encoding]::new($false) }
    [IO.File]::WriteAllText($target, $text.TrimEnd(), $encoding)
}

$removeProjectScript = Join-Path $PSScriptRoot 'Remove-AndroidProject.ps1'
$removeProjectTarget = Join-Path $destinationRoot 'Scripts\Remove-AndroidProject.ps1'
[IO.File]::Copy($removeProjectScript, $removeProjectTarget)

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
# Google OAuth setup for {{Name}}

Create two Android OAuth clients in Google Cloud. The build uploader uses
a separate shared Desktop OAuth client and does not use these Android client IDs.

```text
Name: {{Name}} debug
Package name: {{PackageId}}
SHA-1 certificate fingerprint: {{DebugSha1}}

Name: {{Name}} release
Package name: {{PackageId}}
SHA-1 certificate fingerprint: {{ReleaseSha1}}
```

Google Cloud Clients page: {{SetupUrl}}

This application's secrets are outside Git:

```text
{{SecretsDirectory}}
├─ debug.keystore
├─ release.keystore
└─ signing.properties
```
'@
$setupUrl = if ($releaseCertificate.SetupUrl) { $releaseCertificate.SetupUrl } else { 'pass -GoogleCloudProject to get a link to the Clients page' }
$oauthDocument = $oauthDocument.Replace('{{Name}}', $Name).Replace('{{PackageId}}', $PackageId).Replace('{{DebugSha1}}', $debugCertificate.Sha1).Replace('{{ReleaseSha1}}', $releaseCertificate.Sha1).Replace('{{SecretsDirectory}}', $secretsDirectory).Replace('{{SetupUrl}}', $setupUrl)
[IO.File]::WriteAllText((Join-Path $destinationRoot 'Google-OAuth-setup.md'), $oauthDocument.TrimEnd(), [Text.UTF8Encoding]::new($false))

Write-Host "Application created: $destinationRoot"
Write-Host "Secrets created: $secretsDirectory"
Write-Host 'Run ./build.ps1 build-android -Configuration Debug from that directory.'