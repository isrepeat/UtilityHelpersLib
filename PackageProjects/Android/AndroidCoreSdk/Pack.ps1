param(
    [switch]$NoPause,
    [string]$PackagesFeedPath
)

function Resolve-JavaHome {
    $candidateRoots = @(
        $env:JAVA_HOME,
        $env:JDK_HOME,
        (Join-Path $env:ProgramFiles 'Android\Android Studio\jbr'),
        (Join-Path $env:ProgramFiles 'Android\openjdk'),
        (Join-Path $env:ProgramFiles 'Java')
    ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

    foreach ($candidateRoot in $candidateRoots) {
        $homes = @($candidateRoot)
        if (Test-Path -LiteralPath $candidateRoot -PathType Container) {
            $homes += Get-ChildItem -LiteralPath $candidateRoot -Directory -ErrorAction SilentlyContinue |
                Sort-Object Name -Descending |
                ForEach-Object FullName
        }

        foreach ($javaHomeCandidate in $homes) {
            if (Test-Path -LiteralPath (Join-Path $javaHomeCandidate 'bin\java.exe') -PathType Leaf) {
                return $javaHomeCandidate
            }
        }
    }

    throw 'Java JDK was not found. Set JAVA_HOME or install Android Studio / Android OpenJDK.'
}

function Resolve-AndroidSdk {
    $candidateRoots = @(
        $env:ANDROID_HOME,
        $env:ANDROID_SDK_ROOT,
        (Join-Path $env:LOCALAPPDATA 'Android\Sdk'),
        (Join-Path ${env:ProgramFiles(x86)} 'Android\android-sdk'),
        (Join-Path $env:ProgramFiles 'Android\Sdk')
    ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

    foreach ($candidateRoot in $candidateRoots) {
        if (Test-Path -LiteralPath (Join-Path $candidateRoot 'platform-tools\adb.exe') -PathType Leaf) {
            return $candidateRoot
        }
    }

    throw 'Android SDK was not found. Set ANDROID_HOME or install Android SDK platform tools.'
}

function Get-NextPackageVersion {
    param(
        [Parameter(Mandatory)] [string]$PropertiesPath,
        [Parameter(Mandatory)] [string]$PackagesFeedPath,
        [Parameter(Mandatory)] [string]$PackageGroup,
        [Parameter(Mandatory)] [string]$ArtifactId
    )

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

function Get-GradleProperty {
    param(
        [Parameter(Mandatory)] [string]$PropertiesPath,
        [Parameter(Mandatory)] [string]$Name
    )

    $content = [System.IO.File]::ReadAllText($PropertiesPath)
    $match = [regex]::Match($content, "(?m)^$([regex]::Escape($Name))=(.+)$")
    if (-not $match.Success) {
        throw "$Name was not found in $PropertiesPath."
    }

    return $match.Groups[1].Value.Trim()
}

$exitCode = 1
$locationChanged = $false

try {
    $repositoryRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
    $wrapperPath = Join-Path $repositoryRoot "Scripts\Android\Gradle\gradlew.bat"

    if (-not (Test-Path -LiteralPath $wrapperPath)) {
        throw "Gradle Wrapper was not found. Create it with: gradle wrapper --gradle-version 8.11.1"
    }

    $propertiesPath = Join-Path $PSScriptRoot 'gradle.properties'
    $feedResolver = Join-Path $repositoryRoot 'Scripts\PowerShell\Resolve-PackagesFeed.ps1'
    $packagesFeedPath = & $feedResolver -FeedPath $PackagesFeedPath
    $packageGroup = Get-GradleProperty $propertiesPath 'packageGroup'
    $nextVersion = Get-NextPackageVersion $propertiesPath $packagesFeedPath $packageGroup 'androidcoresdk'

    $javaHome = Resolve-JavaHome
    $androidSdk = Resolve-AndroidSdk
    $env:JAVA_HOME = $javaHome
    $env:ANDROID_HOME = $androidSdk
    $env:ANDROID_SDK_ROOT = $androidSdk
    $env:Path = "$(Join-Path $javaHome 'bin');$env:Path"

    Push-Location -LiteralPath $PSScriptRoot
    $locationChanged = $true
    Write-Host "Publishing AndroidCoreSdk $nextVersion"
    & $wrapperPath "-PandroidPackagesFeedPath=$packagesFeedPath" "-PpackageVersion=$nextVersion" `
        :androidcoresdk:publishReleasePublicationToAndroidPackagesFeedRepository
    $exitCode = $LASTEXITCODE
    if ($exitCode -eq 0) {
        $packagePath = Join-Path "$packagesFeedPath\$($packageGroup.Replace('.', '\'))\androidcoresdk" `
            "$nextVersion\androidcoresdk-$nextVersion.aar"
        if (-not (Test-Path -LiteralPath $packagePath -PathType Leaf)) {
            throw "Published AAR was not found: $packagePath"
        }

        Write-Host "Package version: $nextVersion"
        Write-Host "Package file: $packagePath"
    }
}
catch {
    Write-Error $_
}
finally {
    if ($locationChanged) {
        Pop-Location
    }

    if (-not $NoPause) {
        Read-Host "Press Enter to close this window" | Out-Null
    }
}

exit $exitCode