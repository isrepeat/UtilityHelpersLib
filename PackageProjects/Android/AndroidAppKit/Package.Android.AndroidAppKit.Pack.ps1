param([switch]$NoPause)

function Resolve-JavaHome {
    $roots = @($env:JAVA_HOME, $env:JDK_HOME, (Join-Path $env:ProgramFiles 'Android\Android Studio\jbr'), (Join-Path $env:ProgramFiles 'Android\openjdk'), (Join-Path $env:ProgramFiles 'Java')) | Where-Object { $_ }
    foreach ($root in $roots) {
        $candidates = @($root)
        if (Test-Path -LiteralPath $root -PathType Container) { $candidates += Get-ChildItem -LiteralPath $root -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | ForEach-Object FullName }
        foreach ($candidate in $candidates) { if (Test-Path -LiteralPath (Join-Path $candidate 'bin\java.exe') -PathType Leaf) { return $candidate } }
    }
    throw 'Java JDK was not found. Set JAVA_HOME or install Android Studio / Android OpenJDK.'
}

function Resolve-AndroidSdk {
    $roots = @($env:ANDROID_HOME, $env:ANDROID_SDK_ROOT, (Join-Path $env:LOCALAPPDATA 'Android\Sdk'), (Join-Path ${env:ProgramFiles(x86)} 'Android\android-sdk'), (Join-Path $env:ProgramFiles 'Android\Sdk')) | Where-Object { $_ }
    foreach ($root in $roots) { if (Test-Path -LiteralPath (Join-Path $root 'platform-tools\adb.exe') -PathType Leaf) { return $root } }
    throw 'Android SDK was not found. Set ANDROID_HOME or install Android SDK platform tools.'
}

function Get-GradleProperty {
    param([string]$PropertiesPath, [string]$Name)
    $match = [regex]::Match([System.IO.File]::ReadAllText($PropertiesPath), "(?m)^$([regex]::Escape($Name))=(.+)$")
    if (-not $match.Success) { throw "$Name was not found in $PropertiesPath." }
    return $match.Groups[1].Value.Trim()
}

function Get-NextPackageVersion {
    param([string]$PropertiesPath, [string]$PackagesFeedPath, [string]$PackageGroup, [string]$ArtifactId)
    $baseVersion = Get-GradleProperty $PropertiesPath 'packageVersionBase'
    if ($baseVersion -notmatch '^\d+\.\d+$') { throw "packageVersionBase must use the major.minor format in $PropertiesPath." }
    $artifactPath = Join-Path (Join-Path $PackagesFeedPath $PackageGroup.Replace('.', '\')) $ArtifactId
    $patches = if (Test-Path -LiteralPath $artifactPath -PathType Container) {
        Get-ChildItem -LiteralPath $artifactPath -Directory | ForEach-Object {
            $match = [regex]::Match($_.Name, "^$([regex]::Escape($baseVersion))\.(\d+)$")
            if ($match.Success) { [int]$match.Groups[1].Value }
        }
    }
    $maximumPatch = ($patches | Measure-Object -Maximum).Maximum
    if ($null -eq $maximumPatch) { $maximumPatch = 0 }
    return "$baseVersion.$($maximumPatch + 1)"
}

$ErrorActionPreference = 'Stop'
$exitCode = 1

try {
    $propertiesPath = Join-Path $PSScriptRoot 'gradle.properties'
    $feed = Get-GradleProperty $propertiesPath 'androidPackagesFeedPath'
    $group = Get-GradleProperty $propertiesPath 'packageGroup'
    $nextVersion = Get-NextPackageVersion $propertiesPath $feed $group 'androidappkit'
    $javaHome = Resolve-JavaHome
    $androidSdk = Resolve-AndroidSdk
    $env:JAVA_HOME = $javaHome
    $env:ANDROID_HOME = $androidSdk
    $env:ANDROID_SDK_ROOT = $androidSdk
    $env:Path = "$(Join-Path $javaHome 'bin');$env:Path"
    $root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
    $wrapper = Join-Path $root 'Scripts\Android\Gradle\gradlew.bat'
    Push-Location $PSScriptRoot
    try {
        & $wrapper "-PpackageVersion=$nextVersion" ':androidappkit:publishReleasePublicationToAndroidPackagesFeedRepository'
        if ($LASTEXITCODE -ne 0) { throw "Gradle exited with $LASTEXITCODE" }
    } finally { Pop-Location }
    $aar = Join-Path "$feed\\$($group.Replace('.', '\\'))\\androidappkit\\$nextVersion" "androidappkit-$nextVersion.aar"
    if (-not (Test-Path -LiteralPath $aar -PathType Leaf)) { throw "Published AAR was not found: $aar" }
    Write-Host "Package file: $aar"
    Write-Host "Published AndroidAppKit $nextVersion"
    $exitCode = 0
} catch { Write-Error $_ }
finally { if (-not $NoPause) { Read-Host 'Press Enter to close this window' | Out-Null } }

exit $exitCode