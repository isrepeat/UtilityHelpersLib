param(
    [switch]$NoPause
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

$exitCode = 1
$locationChanged = $false

try {
    $repositoryRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
    $wrapperPath = Join-Path $repositoryRoot "Scripts\Android\Gradle\gradlew.bat"

    if (-not (Test-Path -LiteralPath $wrapperPath)) {
        throw "Gradle Wrapper was not found. Create it with: gradle wrapper --gradle-version 8.11.1"
    }

    $javaHome = Resolve-JavaHome
    $androidSdk = Resolve-AndroidSdk
    $env:JAVA_HOME = $javaHome
    $env:ANDROID_HOME = $androidSdk
    $env:ANDROID_SDK_ROOT = $androidSdk
    $env:Path = "$(Join-Path $javaHome 'bin');$env:Path"

    Push-Location -LiteralPath $PSScriptRoot
    $locationChanged = $true
    & $wrapperPath :androidcoresdk:publishReleasePublicationToAndroidPackagesFeedRepository
    $exitCode = $LASTEXITCODE
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