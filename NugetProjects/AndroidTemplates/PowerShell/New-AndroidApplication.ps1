[CmdletBinding()]
param(
    [Parameter(Mandatory)] [ValidatePattern('^[A-Z][A-Za-z0-9]*$')] [string]$Name,
    [Parameter(Mandatory)] [ValidatePattern('^[a-z][a-z0-9]*(\.[a-z][a-z0-9]*)+$')] [string]$PackageId,
    [Parameter(Mandatory)] [string]$Destination,
    [string]$BuildToolsSource,
    [string]$NativePackageSource,
    [string]$SecretsRoot,
    [string]$DriveOAuthClientPath,
    [string]$DriveTokenPath,
    [string]$PreviewerDebugExecutablePath,
    [string]$PreviewerReleaseExecutablePath,
    [string]$ApkUpdaterProjectRoot,
    [string]$GoogleCloudProject
)

$ErrorActionPreference = 'Stop'
$utf8Encoding = [Text.UTF8Encoding]::new($false)
[Console]::InputEncoding = $utf8Encoding
[Console]::OutputEncoding = $utf8Encoding
$OutputEncoding = $utf8Encoding
. (Join-Path $PSScriptRoot 'Read-AndroidSharedProps.ps1')
$sharedProps = Read-AndroidSharedProps -ProjectRoot $Destination
if (-not $PSBoundParameters.ContainsKey('BuildToolsSource')) {
    $BuildToolsSource = $sharedProps.Paths.PackagesFeed
}
if (-not $PSBoundParameters.ContainsKey('NativePackageSource')) {
    $NativePackageSource = $sharedProps.Paths.PackagesFeed
}
if (-not $PSBoundParameters.ContainsKey('PreviewerDebugExecutablePath')) {
    $PreviewerDebugExecutablePath = $sharedProps.Paths.PreviewerDebugExecutablePath
}
if (-not $PSBoundParameters.ContainsKey('PreviewerReleaseExecutablePath')) {
    $PreviewerReleaseExecutablePath = $sharedProps.Paths.PreviewerReleaseExecutablePath
}
if (-not $PSBoundParameters.ContainsKey('SecretsRoot')) {
    $SecretsRoot = $sharedProps.Paths.SecretsRoot
}
if (-not $PSBoundParameters.ContainsKey('DriveOAuthClientPath')) {
    $DriveOAuthClientPath = $sharedProps.Paths.DriveOAuthClientPath
}
if (-not $PSBoundParameters.ContainsKey('DriveTokenPath')) {
    $DriveTokenPath = $sharedProps.Paths.DriveTokenPath
}
if (-not $PSBoundParameters.ContainsKey('ApkUpdaterProjectRoot')) {
    $ApkUpdaterProjectRoot = $sharedProps.Paths.ApkUpdaterProjectRoot
}
if (-not $PSBoundParameters.ContainsKey('GoogleCloudProject')) {
    $GoogleCloudProject = $sharedProps.Properties.GoogleCloudProject
}
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
$sharedSecretsDirectory = Join-Path ([IO.Path]::GetFullPath($SecretsRoot)) 'shared'
$signingPropertiesPath = $sharedProps.Paths.SigningProperties
if ([IO.Path]::GetFullPath((Split-Path -Parent $signingPropertiesPath)) -ne $sharedSecretsDirectory) {
    throw 'Paths.SigningProperties must be located in the shared directory under Paths.SecretsRoot.'
}

$updaterBuildScript = $null
if (-not [string]::IsNullOrWhiteSpace($ApkUpdaterProjectRoot)) {
    $updaterBuildScript = Join-Path ([IO.Path]::GetFullPath($ApkUpdaterProjectRoot)) 'build.ps1'
    if (-not (Test-Path -LiteralPath $updaterBuildScript -PathType Leaf)) {
        throw "ApkUpdater build script was not found: $updaterBuildScript"
    }
}

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
    $text = $text.Replace('{{ApkUpdaterProjectRoot}}', $ApkUpdaterProjectRoot.Replace("'", "''"))
    $text = $text.Replace('{{DriveOAuthClientPath}}', $DriveOAuthClientPath.Replace("'", "''"))
    $text = $text.Replace('{{DriveTokenPath}}', $DriveTokenPath.Replace("'", "''"))
    $text = $text.Replace('{{PreviewerDebugExecutablePath}}', $PreviewerDebugExecutablePath.Replace("'", "''"))
    $text = $text.Replace('{{PreviewerReleaseExecutablePath}}', $PreviewerReleaseExecutablePath.Replace("'", "''"))
    $encoding = if ($file.Extension -eq '.ps1') { [Text.UTF8Encoding]::new($true) } else { [Text.UTF8Encoding]::new($false) }
    [IO.File]::WriteAllText($target, $text.TrimEnd(), $encoding)
}

# Общие скрипты уже включены в template_app при упаковке NuGet из одного исходника.

[IO.Directory]::CreateDirectory($secretsDirectory) | Out-Null
[IO.Directory]::CreateDirectory($sharedSecretsDirectory) | Out-Null
$oauthSetup = Join-Path $PSScriptRoot 'Open-AndroidOAuthClientSetup.ps1'
if (-not (Test-Path -LiteralPath $signingPropertiesPath -PathType Leaf)) {
    $debugKeystore = Join-Path $sharedSecretsDirectory 'debug.keystore'
    $releaseKeystore = Join-Path $sharedSecretsDirectory 'release.keystore'
    $debugPassword = New-KeyPassword
    $releasePassword = New-KeyPassword
    $debugCertificate = & $oauthSetup -ClientName "$Name debug" -PackageId $PackageId -KeystorePath $debugKeystore -KeyAlias debug -StorePassword $debugPassword -KeyPassword $debugPassword -CreateKeystore
    $releaseCertificate = & $oauthSetup -ClientName "$Name release" -PackageId $PackageId -KeystorePath $releaseKeystore -KeyAlias release -StorePassword $releasePassword -KeyPassword $releasePassword -CreateKeystore

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
    Write-Host "WARNING: Созданы новые общие ключи Android: $sharedSecretsDirectory. Пересоберите ApkUpdater для Debug и Release с этими ключами и установите соответствующую сборку на устройство, иначе приложения не смогут вызвать Updater из-за несовпадения подписей." -ForegroundColor Yellow
    if ($null -ne $updaterBuildScript) {
        foreach ($updaterConfiguration in @('Debug', 'Release')) {
            Write-Host "Building ApkUpdater $updaterConfiguration with $signingPropertiesPath" -ForegroundColor Green
            & $updaterBuildScript build-android -Configuration $updaterConfiguration -SigningProperties $signingPropertiesPath
        }
        Write-Host 'ApkUpdater Debug и Release пересобраны. Установите соответствующий APK на устройство.' -ForegroundColor Green
    } else {
        Write-Host 'WARNING: Для автоматической сборки укажите -ApkUpdaterProjectRoot. Для ручной сборки выполните команды в репозитории ApkUpdater:' -ForegroundColor Yellow
        Write-Host "& '.\build.ps1' build-android -Configuration Debug -SigningProperties '$signingPropertiesPath'" -ForegroundColor Yellow
        Write-Host "& '.\build.ps1' build-android -Configuration Release -SigningProperties '$signingPropertiesPath'" -ForegroundColor Yellow
    }
}
$sharedSigning = ConvertFrom-StringData ([IO.File]::ReadAllText($signingPropertiesPath))
$debugCertificate = & $oauthSetup -ClientName "$Name debug" -PackageId $PackageId -KeystorePath $sharedSigning.debugStoreFile -KeyAlias $sharedSigning.debugKeyAlias -StorePassword $sharedSigning.debugStorePassword -KeyPassword $sharedSigning.debugKeyPassword
$releaseCertificate = & $oauthSetup -ClientName "$Name release" -PackageId $PackageId -KeystorePath $sharedSigning.storeFile -KeyAlias $sharedSigning.keyAlias -StorePassword $sharedSigning.storePassword -KeyPassword $sharedSigning.keyPassword -GoogleCloudProject $GoogleCloudProject -OpenBrowser
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
$oauthDocument = $oauthDocument.Replace('{{Name}}', $Name).Replace('{{PackageId}}', $PackageId).Replace('{{DebugSha1}}', $debugCertificate.Sha1).Replace('{{ReleaseSha1}}', $releaseCertificate.Sha1).Replace('{{SecretsDirectory}}', $sharedSecretsDirectory).Replace('{{SetupUrl}}', $setupUrl)
[IO.File]::WriteAllText((Join-Path $destinationRoot 'Google-OAuth-setup.md'), $oauthDocument.TrimEnd(), [Text.UTF8Encoding]::new($false))

Write-Host "Application created: $destinationRoot"
Write-Host "Secrets created: $secretsDirectory"
Write-Host 'Run ./build.ps1 build-android -Configuration Debug from that directory.'