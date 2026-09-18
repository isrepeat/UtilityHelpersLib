[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$UtilityHelpersRoot,

    [string]$AndroidNdkRoot,

    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',

    [string[]]$AndroidAbis = @('arm64-v8a'),

    [int]$AndroidApi = 24,

    [switch]$SkipPackage
)

$ErrorActionPreference = 'Stop'
$stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

# Корень UtilityHelpersLib передаёт запускающий .cmd через %~dp0. Поэтому
# расположение этого файла не участвует в вычислении путей упаковки.
$utilityRoot = (Resolve-Path -LiteralPath $UtilityHelpersRoot).Path
$modulePath = Join-Path $utilityRoot 'Scripts\PowerShell\Modules\MessagingModule\MessagingModule.psm1'
Import-Module -Name $modulePath -Prefix m:: -ErrorAction Stop

# Ищем MSBuild через vswhere, чтобы не зависеть от конкретной редакции Visual Studio.
function Find-MsBuild {
    $vsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vsWhere)) {
        m::MessageError "vswhere.exe was not found: $vsWhere"
        throw "vswhere.exe was not found: $vsWhere"
    }
    $path = & $vsWhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
    if ([string]::IsNullOrWhiteSpace($path) -or -not (Test-Path -LiteralPath $path)) {
        m::MessageError 'MSBuild.exe was not found in the installed Visual Studio instances.'
        throw 'MSBuild.exe was not found in the installed Visual Studio instances.'
    }
    return $path
}

# Android-проекты конфигурируются CMake, поэтому CMake должен быть доступен из PATH.
function Find-CMake {
    $command = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if ($null -ne $command) {
        return $command.Source
    }

    $visualStudioCMake = Join-Path ${env:ProgramFiles} 'Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    if (Test-Path -LiteralPath $visualStudioCMake) {
        return $visualStudioCMake
    }

    m::MessageError 'cmake.exe was not found. Install CMake or add it to PATH.'
    throw 'cmake.exe was not found. Install CMake or add it to PATH.'
}

# Visual Studio поставляет Ninja рядом с интеграцией CMake, но не добавляет его
# в PATH. Передаём найденный executable явно, чтобы Android-конфигурация не
# зависела от настройки пользовательского окружения.
function Find-Ninja {
    $command = Get-Command ninja.exe -ErrorAction SilentlyContinue
    if ($null -ne $command) {
        return $command.Source
    }

    $visualStudioNinja = Join-Path ${env:ProgramFiles} 'Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
    if (Test-Path -LiteralPath $visualStudioNinja) {
        return $visualStudioNinja
    }

    m::MessageError 'ninja.exe was not found. Install Ninja or add it to PATH.'
    throw 'ninja.exe was not found. Install Ninja or add it to PATH.'
}

# ANGLE is a build-time dependency of XamlRuntime. Keep its manifest next to
# this package and install vcpkg artifacts under XamlRuntime's ignored temp
# directory, never in a separate Angle NuGet project.
function Find-Vcpkg {
    if (-not [string]::IsNullOrWhiteSpace($env:VSInstallDir)) {
        $vcpkg = Join-Path ${env:VSInstallDir} 'VC\vcpkg\vcpkg.exe'
        if (Test-Path -LiteralPath $vcpkg) {
            return $vcpkg
        }
    }

    $defaultVcpkg = Join-Path ${env:ProgramFiles} 'Microsoft Visual Studio\18\Community\VC\vcpkg\vcpkg.exe'
    if (Test-Path -LiteralPath $defaultVcpkg) {
        return $defaultVcpkg
    }

    m::MessageError 'vcpkg.exe was not found in the installed Visual Studio instance.'
    throw 'vcpkg.exe was not found in the installed Visual Studio instance.'
}

# У явного параметра наивысший приоритет. Затем проверяем переменные окружения
# и, как последний вариант, NDK из Android SDK текущего MobileClock checkout.
function Resolve-AndroidNdkRoot([string]$utilityRoot, [string]$requestedPath) {
    $candidates = [System.Collections.Generic.List[string]]::new()
    if (-not [string]::IsNullOrWhiteSpace($requestedPath)) {
        $candidates.Add($requestedPath)
    }
    if (-not [string]::IsNullOrWhiteSpace($env:ANDROID_NDK_HOME)) {
        $candidates.Add($env:ANDROID_NDK_HOME)
    }
    if (-not [string]::IsNullOrWhiteSpace($env:ANDROID_NDK_ROOT)) {
        $candidates.Add($env:ANDROID_NDK_ROOT)
    }
    $localAndroidNdkDirectory = Join-Path $env:LOCALAPPDATA 'Android\Sdk\ndk'
    if (Test-Path -LiteralPath $localAndroidNdkDirectory) {
        Get-ChildItem -LiteralPath $localAndroidNdkDirectory -Directory | Sort-Object Name -Descending | ForEach-Object {
            $candidates.Add($_.FullName)
        }
    }
    $mobileClockRoot = Split-Path -Parent $utilityRoot
    $localProperties = Join-Path $mobileClockRoot 'MobileClock.Android\local.properties'
    if (Test-Path -LiteralPath $localProperties) {
        $sdkDirectory = (Get-Content -LiteralPath $localProperties | Where-Object { $_ -match '^sdk\.dir=' } | Select-Object -First 1) -replace '^sdk\.dir=', ''
        if (-not [string]::IsNullOrWhiteSpace($sdkDirectory)) {
            $ndkDirectory = Join-Path $sdkDirectory 'ndk'
            if (Test-Path -LiteralPath $ndkDirectory) {
                Get-ChildItem -LiteralPath $ndkDirectory -Directory | Sort-Object Name -Descending | ForEach-Object {
                    $candidates.Add($_.FullName)
                }
            }
        }
    }
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath (Join-Path $candidate 'build\cmake\android.toolchain.cmake')) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    $errorMessage = 'Android NDK was not found. Pass -AndroidNdkRoot or set ANDROID_NDK_HOME, ANDROID_NDK_ROOT, or sdk.dir in MobileClock.Android/local.properties.'
    m::MessageError $errorMessage
    throw $errorMessage
}

# XamlRuntime.nuspec is the single source of truth for the package version.
# Calculate the next patch version before packing, but write it back only after
# MSBuild has created the package successfully, so a failed Pack never skips a version.
function Get-NextPackageVersion([string]$nuspecPath) {
    $nuspecContent = Get-Content -LiteralPath $nuspecPath -Raw
    $startTag = '<version>'
    $endTag = '</version>'
    $startIndex = $nuspecContent.IndexOf($startTag, [System.StringComparison]::Ordinal)
    $endIndex = $nuspecContent.IndexOf($endTag, [System.StringComparison]::Ordinal)
    if (($startIndex -lt 0) -or ($endIndex -lt 0) -or ($endIndex -le $startIndex)) {
        throw "NuGet version element was not found in $nuspecPath"
    }

    $versionStartIndex = $startIndex + $startTag.Length
    $versionLength = $endIndex - $versionStartIndex
    $previousVersionText = $nuspecContent.Substring($versionStartIndex, $versionLength).Trim()
    $previousVersion = $null
    if (-not [System.Version]::TryParse($previousVersionText, [ref]$previousVersion)) {
        throw "NuGet version is invalid in ${nuspecPath}: $previousVersionText"
    }

    $patch = if ($previousVersion.Build -lt 0) { 1 } else { $previousVersion.Build + 1 }
    $nextVersion = "$($previousVersion.Major).$($previousVersion.Minor).$patch"
    $nextNuspecContent = $nuspecContent.Remove($versionStartIndex, $versionLength).Insert($versionStartIndex, $nextVersion)
    return [PSCustomObject]@{
        Version = $nextVersion
        NuspecContent = $nextNuspecContent
    }
}

$runtimeRoot = Join-Path $utilityRoot 'NugetProjects\XamlRuntime'
$packagingRoot = Join-Path $runtimeRoot 'Nuget\XamlRuntime.Package'
# Временная структура пакета лежит рядом с packaging-скриптом, но её имя
# отличает generated staging от исходных папок Nuget и готового feed-а.
$stagingRoot = Join-Path $packagingRoot '!NUGET_STAGING'
$nativeBuildRoot = Join-Path $utilityRoot 'NugetProjects\!NUGET_TMP\Build'
$angleManifestRoot = Join-Path $packagingRoot 'ThirdParty\ANGLE'
$angleRecipesSourceRoot = Join-Path $angleManifestRoot 'CustomRecipes'
$angleOverlayPortsRoot = Join-Path $packagingRoot '!NUGET_TMP\ANGLE\CustomRecipes'
$angleInstallRoot = Join-Path $runtimeRoot '!NUGET_TMP\ANGLE\vcpkg_installed'
$anglePackageRoot = Join-Path $angleInstallRoot 'x64-windows'
$nuspecPath = Join-Path $packagingRoot 'XamlRuntime.nuspec'
$packageVersion = Get-NextPackageVersion $nuspecPath
# UH_NUGET_FEED is shared with NugetProjects/Directory.Build.props, so direct
# NuGet CLI packing and MSBuild package projects publish to the same feed.
$feedRoot = if ([string]::IsNullOrWhiteSpace($env:UH_NUGET_FEED)) { 'C:\NugetFeed' } else { $env:UH_NUGET_FEED }
$Version = $packageVersion.Version
$packageConfigurations = @('Debug', 'Release')
m::Message -color Blue -text "XamlRuntime package [Debug, Release, version $Version]"
$msBuild = Find-MsBuild
$cmake = Find-CMake
$ninja = Find-Ninja
$vcpkg = Find-Vcpkg
$AndroidNdkRoot = Resolve-AndroidNdkRoot $utilityRoot $AndroidNdkRoot

if (-not (Test-Path -LiteralPath (Join-Path $angleManifestRoot 'vcpkg.json'))) {
    m::MessageError "ANGLE vcpkg manifest was not found: $angleManifestRoot"
    throw "ANGLE vcpkg manifest was not found: $angleManifestRoot"
}
if (-not (Test-Path -LiteralPath (Join-Path $angleRecipesSourceRoot 'angle\portfile.cmake'))) {
    m::MessageError "ANGLE custom recipe was not found: $angleRecipesSourceRoot"
    throw "ANGLE custom recipe was not found: $angleRecipesSourceRoot"
}

# Staging-дерево является единственным источником содержимого пакета. Его
# всегда пересоздаём, чтобы в .nupkg не попали файлы от предыдущей сборки.
m::MessageAction 'Prepare package staging directory...'
Remove-Item -LiteralPath $stagingRoot -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $stagingRoot, $feedRoot -Force | Out-Null

# Patch-формат требует завершающий перевод строки. В репозитории текстовые
# файлы намеренно не имеют завершающего whitespace, поэтому перед vcpkg
# готовим из рецепта временную рабочую копию с корректным окончанием patch-файлов.
Remove-Item -LiteralPath $angleOverlayPortsRoot -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path (Split-Path -Parent $angleOverlayPortsRoot) -Force | Out-Null
Copy-Item -LiteralPath $angleRecipesSourceRoot -Destination $angleOverlayPortsRoot -Recurse -Force
Get-ChildItem -LiteralPath $angleOverlayPortsRoot -Filter '*.patch' -File -Recurse | ForEach-Object {
    $patchContent = [System.IO.File]::ReadAllText($_.FullName).TrimEnd([char]13, [char]10)
    [System.IO.File]::WriteAllText($_.FullName, "$patchContent`n", [System.Text.UTF8Encoding]::new($false))
}

m::MessageAction 'Install ANGLE through the XamlRuntime vcpkg manifest...'
& $vcpkg install "--x-manifest-root=$angleManifestRoot" "--overlay-ports=$angleOverlayPortsRoot" "--x-install-root=$angleInstallRoot" '--triplet' 'x64-windows'
if ($LASTEXITCODE -ne 0) {
    m::MessageError 'vcpkg failed to install ANGLE.'
    throw 'vcpkg failed to install ANGLE.'
}

$windowsProjects = @(
    'Helpers\Helpers.Logging\Helpers.Logging.vcxproj',
    'NugetProjects\XamlRuntime\Nuget\XamlRuntime\XamlRuntime.vcxproj',
    'NugetProjects\XamlRuntime\Nuget\OpenGLESRenderer\OpenGLESRenderer.vcxproj',
    'NugetProjects\XamlRuntime\Nuget\XamlCompiler\XamlCompiler.vcxproj'
)
# Нативные проекты собираются в изолированную директорию !NUGET_TMP, чтобы
# packaging не менял обычные выходные каталоги разработки. Один пакет содержит
# обе Windows-конфигурации: Debug-потребитель не может корректно линковать
# Release-статические библиотеки, и наоборот.
foreach ($buildConfiguration in $packageConfigurations) {
    m::MessageAction "Build Windows native artifacts [$buildConfiguration]..."
    foreach ($relativeProjectPath in $windowsProjects) {
        $projectPath = Join-Path $utilityRoot $relativeProjectPath
        m::Message -color DarkGray -text $relativeProjectPath
        & $msBuild $projectPath "/t:Build" "/p:Configuration=$buildConfiguration" '/p:Platform=x64' '/p:XamlRuntimePackageBuild=true' '-verbosity:minimal'
        if ($LASTEXITCODE -ne 0) {
            m::MessageError "Windows build failed: $projectPath"
            throw "Windows build failed: $projectPath"
        }
    }

    # Windows-библиотеки, компилятор XAML и весь runtime ANGLE кладём вместе:
    # потребителю достаточно подключить один пакет XamlRuntime.
    $windowsOutput = Join-Path $stagingRoot "runtimes\win-x64\native\$buildConfiguration"
    $angleRuntimeDirectory = if ($buildConfiguration -eq 'Debug') { Join-Path $anglePackageRoot 'debug' } else { $anglePackageRoot }
    $zRuntimeFile = if ($buildConfiguration -eq 'Debug') { 'zd.dll' } else { 'z.dll' }
    New-Item -ItemType Directory -Path $windowsOutput, (Join-Path $stagingRoot 'tools\win-x64') -Force | Out-Null
    Copy-Item (Join-Path $nativeBuildRoot "$buildConfiguration\x64\XamlRuntime\XamlRuntime.lib") $windowsOutput
    Copy-Item (Join-Path $nativeBuildRoot "$buildConfiguration\x64\OpenGLESRenderer\OpenGLESRenderer.lib") $windowsOutput
    Copy-Item (Join-Path $nativeBuildRoot "$buildConfiguration\x64\Helpers.Logging\Helpers.Logging.lib") $windowsOutput
    Copy-Item (Join-Path $nativeBuildRoot "$buildConfiguration\x64\XamlCompiler\XamlCompiler.exe") (Join-Path $stagingRoot 'tools\win-x64')
    Copy-Item (Join-Path $angleRuntimeDirectory 'lib\libEGL.lib') $windowsOutput
    Copy-Item (Join-Path $angleRuntimeDirectory 'lib\libGLESv2.lib') $windowsOutput
    Copy-Item (Join-Path $angleRuntimeDirectory 'bin\libEGL.dll') $windowsOutput
    Copy-Item (Join-Path $angleRuntimeDirectory 'bin\libGLESv2.dll') $windowsOutput
    Copy-Item (Join-Path $angleRuntimeDirectory "bin\$zRuntimeFile") $windowsOutput
    if ($buildConfiguration -eq 'Debug') {
        # Debug archive symbols are useful while stepping from a consumer DLL
        # into our XamlRuntime code. Keep our PDBs in the package; ANGLE PDBs
        # remain disabled until its third-party source debugging is needed.
        Copy-Item (Join-Path $nativeBuildRoot 'Debug\x64\Helpers.Logging\Helpers.Logging.pdb') $windowsOutput
        Copy-Item (Join-Path $nativeBuildRoot 'Debug\x64\XamlRuntime\XamlRuntime.pdb') $windowsOutput
        Copy-Item (Join-Path $nativeBuildRoot 'Debug\x64\OpenGLESRenderer\OpenGLESRenderer.pdb') $windowsOutput
        Copy-Item (Join-Path $nativeBuildRoot 'Debug\x64\XamlCompiler\XamlCompiler.pdb') (Join-Path $stagingRoot 'tools\win-x64')
        # Copy-Item (Join-Path $angleRuntimeDirectory 'bin\*.pdb') $windowsOutput
    }
}

# Для каждой ABI создаётся независимый CMake build-dir. CMake install сразу
# раскладывает статические Android-библиотеки по стандартному runtimes/<RID>.
$abiToRid = @{ 'arm64-v8a' = 'android-arm64'; 'armeabi-v7a' = 'android-arm'; 'x86_64' = 'android-x64' }
foreach ($buildConfiguration in $packageConfigurations) {
    foreach ($abi in $AndroidAbis) {
        if (-not $abiToRid.ContainsKey($abi)) {
            m::MessageError "Unsupported Android ABI: $abi"
            throw "Unsupported Android ABI: $abi"
        }
        m::MessageAction "Build Android artifacts [$abi, $buildConfiguration]..."
        $buildDirectory = Join-Path $runtimeRoot "!NUGET_TMP\Android\$abi\$buildConfiguration"
        $installDirectory = Join-Path $stagingRoot "runtimes\$($abiToRid[$abi])\native\$buildConfiguration"
        & $cmake '-S' (Join-Path $packagingRoot 'Android') '-B' $buildDirectory '-G' 'Ninja' "-DCMAKE_MAKE_PROGRAM=$ninja" "-DCMAKE_TOOLCHAIN_FILE=$AndroidNdkRoot\build\cmake\android.toolchain.cmake" "-DANDROID_ABI=$abi" "-DANDROID_PLATFORM=android-$AndroidApi" "-DCMAKE_BUILD_TYPE=$buildConfiguration"
        if ($LASTEXITCODE -ne 0) {
            m::MessageError "Android CMake configure failed for $abi"
            throw "Android CMake configure failed for $abi"
        }
        & $cmake '--build' $buildDirectory '--target' 'utility_helpers_xaml_runtime' 'utility_helpers_open_gles_renderer'
        if ($LASTEXITCODE -ne 0) {
            m::MessageError "Android CMake build failed for $abi"
            throw "Android CMake build failed for $abi"
        }
        & $cmake '--install' $buildDirectory '--prefix' $installDirectory
        if ($LASTEXITCODE -ne 0) {
            m::MessageError "Android CMake install failed for $abi"
            throw "Android CMake install failed for $abi"
        }
    }
}

# Заголовки и файлы интеграции не компилируются: они копируются в build/native,
# откуда их подхватывают XamlRuntime.targets и XamlRuntimeConfig.cmake.
# ANGLE собирается из внутреннего vcpkg manifest и поставляется вместе с
# лицензией, полученной из vcpkg installed tree.
Copy-Item (Join-Path $runtimeRoot 'Nuget\XamlRuntime\XamlRuntime.Shared\XamlRuntime') (Join-Path $stagingRoot 'build\native\include\XamlRuntime') -Recurse
Copy-Item (Join-Path $runtimeRoot 'Nuget\OpenGLESRenderer\OpenGLESRenderer.Shared\ESRenderer') (Join-Path $stagingRoot 'build\native\include\ESRenderer') -Recurse
Copy-Item (Join-Path $runtimeRoot 'Nuget\OpenGLESRenderer\ThirdParty') (Join-Path $stagingRoot 'build\native\include\ThirdParty') -Recurse
Copy-Item (Join-Path $utilityRoot 'Helpers\Helpers.Logging\Helpers.Logging') (Join-Path $stagingRoot 'build\native\include\Helpers.Logging') -Recurse
Copy-Item (Join-Path $utilityRoot '3rdParty\Spdlog\Spdlog.Shared\Spdlog\spdlog') (Join-Path $stagingRoot 'build\native\include\spdlog') -Recurse
Copy-Item -Path (Join-Path $anglePackageRoot 'include\*') -Destination (Join-Path $stagingRoot 'build\native\include') -Recurse
New-Item -ItemType Directory -Path (Join-Path $stagingRoot 'licenses') -Force | Out-Null
Copy-Item (Join-Path $anglePackageRoot 'share\angle\copyright') (Join-Path $stagingRoot 'licenses\ANGLE.copyright')
Copy-Item (Join-Path $packagingRoot 'build\native\XamlRuntime.targets') (Join-Path $stagingRoot 'build\native\XamlRuntime.targets')
New-Item -ItemType Directory -Path (Join-Path $stagingRoot 'build\native\cmake') -Force | Out-Null
Copy-Item (Join-Path $packagingRoot 'cmake\XamlRuntimeConfig.cmake') (Join-Path $stagingRoot 'build\native\cmake\XamlRuntimeConfig.cmake')
Copy-Item (Join-Path $runtimeRoot 'README.md') (Join-Path $stagingRoot 'README.md')
[System.IO.File]::WriteAllText((Join-Path $stagingRoot 'XamlRuntime.nuspec'), $packageVersion.NuspecContent, [System.Text.UTF8Encoding]::new($false))

# Параметр -SkipPackage оставлен для диагностики staging-дерева без создания
# .nupkg; штатный release-скрипт этот режим не использует.
if (-not $SkipPackage) {
    m::MessageAction 'Create NuGet package through MSBuild...'
    & $msBuild (Join-Path $packagingRoot 'XamlRuntime.Package.vcxproj') '/t:Pack' "/p:Configuration=$Configuration" '/p:Platform=x64' "/p:PackageVersion=$Version" "/p:PackageOutputPath=$feedRoot" '-verbosity:minimal'
    if ($LASTEXITCODE -ne 0) {
        m::MessageError 'MSBuild NuGet Pack failed.'
        throw 'MSBuild NuGet Pack failed.'
    }
    $packagePath = Join-Path $feedRoot "XamlRuntime.$Version.nupkg"
    if (-not (Test-Path -LiteralPath $packagePath -PathType Leaf)) {
        m::MessageError "MSBuild completed without creating the expected package: $packagePath"
        throw "MSBuild completed without creating the expected package: $packagePath"
    }
    [System.IO.File]::WriteAllText($nuspecPath, $packageVersion.NuspecContent, [System.Text.UTF8Encoding]::new($false))
    m::Message -color Green -text "Package created: $packagePath"
}

$stopwatch.Stop()
m::Message -color Cyan -text ('Package build time: {0:hh\:mm\:ss\.fff}' -f $stopwatch.Elapsed)