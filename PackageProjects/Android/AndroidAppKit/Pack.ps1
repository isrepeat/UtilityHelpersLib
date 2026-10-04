param(
    [switch]$NoPause,
    [string]$PackagesFeedPath
)

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
    # Полная версия хранится в исходниках; увеличиваем только последнюю часть.
    $content = [System.IO.File]::ReadAllText($PropertiesPath)
    $match = [regex]::Match($content, '(?m)^packageVersion=(\d+(?:\.\d+){2})\r?$')
    if (-not $match.Success) { throw "Full package version is missing in $PropertiesPath." }
    $parts = $match.Groups[1].Value.Split('.')
    $parts[$parts.Length - 1] = ([int]$parts[$parts.Length - 1] + 1).ToString()
    $nextVersion = $parts -join '.'
    $content = $content.Remove($match.Groups[1].Index, $match.Groups[1].Length).Insert($match.Groups[1].Index, $nextVersion)
    [System.IO.File]::WriteAllText($PropertiesPath, $content.TrimEnd(), [System.Text.UTF8Encoding]::new($false))
    return $nextVersion
}

$ErrorActionPreference = 'Stop'
$exitCode = 1

try {
    $propertiesPath = Join-Path $PSScriptRoot 'gradle.properties'
    $root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
    $feedResolver = Join-Path $root 'Scripts\PowerShell\Resolve-PackagesFeed.ps1'
    $feed = & $feedResolver -FeedPath $PackagesFeedPath
    $group = Get-GradleProperty $propertiesPath 'packageGroup'
    $nextVersion = Get-NextPackageVersion $propertiesPath $feed $group 'androidappkit'
    $javaHome = Resolve-JavaHome
    $androidSdk = Resolve-AndroidSdk
    $env:JAVA_HOME = $javaHome
    $env:ANDROID_HOME = $androidSdk
    $env:ANDROID_SDK_ROOT = $androidSdk
    $env:Path = "$(Join-Path $javaHome 'bin');$env:Path"
    $wrapper = Join-Path $root 'Scripts\Android\Gradle\gradlew.bat'
    Push-Location $PSScriptRoot
    try {
        & $wrapper "-PandroidPackagesFeedPath=$feed" "-PpackageVersion=$nextVersion" ':androidappkit:publishReleasePublicationToAndroidPackagesFeedRepository'
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