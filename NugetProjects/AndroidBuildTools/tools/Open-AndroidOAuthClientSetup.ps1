[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string]$ClientName,
    [Parameter(Mandatory)] [ValidatePattern('^[a-z][a-z0-9]*(\.[a-z][a-z0-9]*)+$')] [string]$PackageId,
    [Parameter(Mandatory)] [string]$KeystorePath,
    [Parameter(Mandatory)] [string]$KeyAlias,
    [string]$StorePassword,
    [string]$KeyPassword,
    [string]$GoogleCloudProject,
    [switch]$CreateKeystore,
    [switch]$OpenBrowser
)

$ErrorActionPreference = 'Stop'
Import-Module -Name (Join-Path $PSScriptRoot 'Modules\Module.AndroidBuildTools\Module.AndroidBuildTools.psm1') -ErrorAction Stop
Module.AndroidBuildTools\Initialize-AndroidBuildConsole

$javaHome = Module.AndroidBuildTools\Resolve-AndroidJavaHome
$keyTool = Join-Path $javaHome 'bin\keytool.exe'
$resolvedKeystorePath = [IO.Path]::GetFullPath($KeystorePath)

if (-not (Test-Path -LiteralPath $resolvedKeystorePath -PathType Leaf)) {
    if (-not $CreateKeystore) {
        throw "Keystore not found: $resolvedKeystorePath."
    }
    if (-not $StorePassword -or -not $KeyPassword) {
        throw 'Creating a keystore requires StorePassword and KeyPassword.'
    }

    [IO.Directory]::CreateDirectory((Split-Path -Parent $resolvedKeystorePath)) | Out-Null
    # keytool печатает полученные аргументы. Передаём пароли через временные
    # переменные окружения, чтобы они не попали в вывод сборки.
    $passwordVariableSuffix = [Guid]::NewGuid().ToString('N')
    $storePasswordVariable = "ANDROID_BUILD_TOOLS_STORE_PASSWORD_$passwordVariableSuffix"
    $keyPasswordVariable = "ANDROID_BUILD_TOOLS_KEY_PASSWORD_$passwordVariableSuffix"
    try {
        [Environment]::SetEnvironmentVariable($storePasswordVariable, $StorePassword, 'Process')
        [Environment]::SetEnvironmentVariable($keyPasswordVariable, $KeyPassword, 'Process')
        & $keyTool -genkeypair -keystore $resolvedKeystorePath -alias $KeyAlias -keyalg RSA -keysize 2048 -validity 10000 -storepass:env $storePasswordVariable -keypass:env $keyPasswordVariable -dname "CN=$ClientName,O=Android,C=US" | Out-Host
        if ($LASTEXITCODE -ne 0) {
            throw "keytool could not create the keystore: $LASTEXITCODE."
        }
    } finally {
        [Environment]::SetEnvironmentVariable($storePasswordVariable, $null, 'Process')
        [Environment]::SetEnvironmentVariable($keyPasswordVariable, $null, 'Process')
    }
}

$keyToolArguments = @('-list', '-v', '-keystore', $resolvedKeystorePath, '-alias', $KeyAlias)
$storePasswordVariable = $null
if ($StorePassword) {
    $storePasswordVariable = "ANDROID_BUILD_TOOLS_STORE_PASSWORD_$([Guid]::NewGuid().ToString('N'))"
    [Environment]::SetEnvironmentVariable($storePasswordVariable, $StorePassword, 'Process')
    $keyToolArguments += @('-storepass:env', $storePasswordVariable)
}
try {
    $output = & $keyTool @keyToolArguments 2>&1
} finally {
    if ($storePasswordVariable) {
        [Environment]::SetEnvironmentVariable($storePasswordVariable, $null, 'Process')
    }
}
if ($LASTEXITCODE -ne 0) {
    throw "keytool could not read the certificate: $($output -join [Environment]::NewLine)"
}

$match = [regex]::Match(($output -join [Environment]::NewLine), '(?m)^\s*SHA1:\s*(?<value>[0-9A-F:]+)\s*$')
if (-not $match.Success) {
    throw 'keytool did not return a SHA-1 certificate fingerprint.'
}

$setupUrl = if ($GoogleCloudProject) { "https://console.cloud.google.com/auth/clients?project=$([Uri]::EscapeDataString($GoogleCloudProject))" }
if ($OpenBrowser -and $setupUrl) {
    Start-Process $setupUrl
}

$result = [pscustomobject]@{
    ClientName = $ClientName
    PackageId = $PackageId
    Sha1 = $match.Groups['value'].Value
    KeystorePath = $resolvedKeystorePath
    KeyAlias = $KeyAlias
    SetupUrl = $setupUrl
}
Write-Host 'OAuth client name — copy the next line:'
Write-Host $result.ClientName
Write-Host 'Package ID — copy the next line:'
Write-Host $result.PackageId
Write-Host 'SHA-1 certificate fingerprint — copy the next line:'
Write-Host $result.Sha1
return $result