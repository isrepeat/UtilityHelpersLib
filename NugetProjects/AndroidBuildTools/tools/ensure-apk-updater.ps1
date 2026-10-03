[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string]$ApkUpdaterProjectRoot,
    [Parameter(Mandatory)] [string]$SigningProperties,
    [Parameter(Mandatory)] [string]$JavaHome,
    [Parameter(Mandatory)] [string]$AndroidSdk,
    [ValidateSet('Debug', 'Release')] [string]$Configuration = 'Debug'
)

$ErrorActionPreference = 'Stop'
$updaterRoot = [IO.Path]::GetFullPath($ApkUpdaterProjectRoot)
$buildScript = Join-Path $updaterRoot 'Scripts\PowerShell\build-android.ps1'
if (-not (Test-Path -LiteralPath $buildScript -PathType Leaf)) {
    throw "ApkUpdater build script was not found: $buildScript"
}
$signing = ConvertFrom-StringData ([IO.File]::ReadAllText($SigningProperties))
$prefix = if ($Configuration -eq 'Debug') { 'debug' } else { '' }
$storeKey = if ($prefix) { 'debugStoreFile' } else { 'storeFile' }
$passwordKey = if ($prefix) { 'debugStorePassword' } else { 'storePassword' }
$aliasKey = if ($prefix) { 'debugKeyAlias' } else { 'keyAlias' }
foreach ($key in @($storeKey, $passwordKey, $aliasKey)) {
    if ([string]::IsNullOrWhiteSpace($signing[$key])) {
        throw "Required signing setting is missing: $key"
    }
}
$keytool = Join-Path $JavaHome 'bin\keytool.exe'
$java = Join-Path $JavaHome 'bin\java.exe'
$apksigner = Get-ChildItem -LiteralPath (Join-Path $AndroidSdk 'build-tools') -Directory |
    Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'lib\apksigner.jar') -PathType Leaf } |
    Sort-Object { [version]($_.Name.Split('-')[0]) } -Descending |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $apksigner) {
    throw "apksigner was not found in $AndroidSdk"
}
$apksigner = Join-Path $apksigner 'lib\apksigner.jar'

# Пароль передаётся через временную переменную окружения, а не аргументы процесса.
$previousPassword = $env:APKUPDATER_CERTIFICATE_STORE_PASSWORD
$env:APKUPDATER_CERTIFICATE_STORE_PASSWORD = $signing[$passwordKey]
try {
    $certificateOutput = & $keytool '-J-Duser.language=en' '-J-Duser.country=US' -list -v -keystore $signing[$storeKey] -alias $signing[$aliasKey] -storepass:env APKUPDATER_CERTIFICATE_STORE_PASSWORD 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw 'Unable to read the shared signing certificate with keytool.'
    }
} finally {
    $env:APKUPDATER_CERTIFICATE_STORE_PASSWORD = $previousPassword
}
$certificateMatch = [regex]::Match(($certificateOutput -join "`n"), 'SHA256:\s*([0-9A-Fa-f:]+)')
if (-not $certificateMatch.Success) {
    throw 'Shared certificate SHA-256 fingerprint was not found.'
}
$expectedFingerprint = $certificateMatch.Groups[1].Value.Replace(':', '').ToLowerInvariant()
$variant = $Configuration.ToLowerInvariant()
$apkPath = Join-Path $updaterRoot "Build\ApkUpdater.Android\outputs\apk\$variant\ApkUpdater.Android-$variant.apk"

function Test-UpdaterCertificate {
    if (-not (Test-Path -LiteralPath $apkPath -PathType Leaf)) {
        return $false
    }
    $output = & $java -jar $apksigner verify --print-certs $apkPath 2>&1
    if ($LASTEXITCODE -ne 0) {
        return $false
    }
    $fingerprints = [regex]::Matches(($output -join "`n"), '(?m)^Signer #\d+ certificate SHA-256 digest:\s*([0-9A-Fa-f]+)\s*$')
    return $fingerprints.Count -eq 1 -and $fingerprints[0].Groups[1].Value.ToLowerInvariant() -eq $expectedFingerprint
}

if (Test-UpdaterCertificate) {
    Write-Host "ApkUpdater $Configuration already uses the shared signing certificate."
    return
}
Write-Host "WARNING: ApkUpdater $Configuration APK отсутствует, повреждён или подписан другим ключом. Пересобираем с общими ключами." -ForegroundColor Yellow
& $buildScript -Configuration $Configuration -SigningProperties $SigningProperties
if (-not (Test-UpdaterCertificate)) {
    throw "Rebuilt ApkUpdater certificate does not match the shared key: $apkPath"
}
Write-Host "ApkUpdater $Configuration готов: $apkPath. Установите этот APK на устройство." -ForegroundColor Green