[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$ProjectRoot,

    [switch]$BuildOnly,

    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [int]$ParentProcessId = 0
)

$ErrorActionPreference = 'Stop'
$utf8Encoding = [System.Text.UTF8Encoding]::new($false)
[Console]::InputEncoding = $utf8Encoding
[Console]::OutputEncoding = $utf8Encoding
$OutputEncoding = $utf8Encoding

function Initialize-VisualStudioEnvironment {
    param(
        [Parameter(Mandatory = $true)]
        [string]$VisualStudioRoot
    )

    # Передаём окружение Visual Studio компилятору, запущенному через Ninja.
    $developerCommand = Join-Path $VisualStudioRoot 'Common7\Tools\VsDevCmd.bat'
    if (-not (Test-Path -LiteralPath $developerCommand)) {
        throw "Visual Studio developer command was not found: $developerCommand"
    }

    $environmentLines = & cmd.exe /c "`"$developerCommand`" -arch=x64 -host_arch=x64 >nul && set"
    foreach ($environmentLine in $environmentLines) {
        $separatorIndex = $environmentLine.IndexOf('=')
        if ($separatorIndex -gt 0) {
            $name = $environmentLine.Substring(0, $separatorIndex)
            $value = $environmentLine.Substring($separatorIndex + 1)
            Set-Item -LiteralPath "Env:$name" -Value $value
        }
    }
}

try {
    Import-Module -Name (Join-Path $PSScriptRoot 'Modules\Module.AndroidBuildTools\Module.AndroidBuildTools.psm1') -ErrorAction Stop
    $config = Module.AndroidBuildTools\Read-AndroidBuildConfiguration $ProjectRoot
    $tools = Module.AndroidBuildTools\Resolve-AndroidBuildTools
    $generateXamlScript = Join-Path $PSScriptRoot 'generate-xaml.ps1'
    $artifactDirectory = Join-Path $projectRoot $config.Preview.ArtifactDirectory
    $previewerRoot = Join-Path $projectRoot $config.Preview.Root
    $projectFile = Join-Path $previewerRoot $config.Preview.ProjectFile
    $previewer = Join-Path $previewerRoot $config.Preview.Executable.Replace('{Configuration}', $Configuration)
    $plugin = Join-Path $artifactDirectory $config.Preview.Plugin.Replace('{Configuration}', $Configuration)
    $binaryLogDirectory = Join-Path $artifactDirectory 'Logs'
    $binaryLogName = "android-app-previewer-{0:yyyyMMdd-HHmmss}.binlog" -f [DateTime]::Now
    $binaryLogPath = Join-Path $binaryLogDirectory $binaryLogName
    $visualStudioMsBuild = Join-Path $tools.VisualStudio 'MSBuild\Current\Bin\MSBuild.exe'
    $visualStudioCmake = $tools.CMake
    $visualStudioNinja = $tools.Ninja
    $cmakeBuildDirectory = Join-Path $artifactDirectory 'Intermediate\CMake'


    if (-not (Test-Path $projectFile)) {
        throw "AndroidAppPreviewer was not found at $previewerRoot. Check Preview.Root in android-build.psd1."
    }

    if (Test-Path $visualStudioMsBuild) {
        $msBuild = $visualStudioMsBuild
    } else {
        $msBuild = (Get-Command MSBuild.exe -ErrorAction Stop).Source
    }
    Initialize-VisualStudioEnvironment -VisualStudioRoot $tools.VisualStudio

    if ($ParentProcessId -gt 0) {
        $parentProcess = Get-Process -Id $ParentProcessId -ErrorAction SilentlyContinue
        if ($null -ne $parentProcess) {
            $parentProcess.WaitForExit()
        }
    }

    & $generateXamlScript -ProjectRoot $ProjectRoot

    New-Item -ItemType Directory -Path $binaryLogDirectory -Force | Out-Null
    Write-Host "==> Rebuilding Application preview plugin $Configuration x64"
    if (Test-Path $visualStudioCmake) {
        $cmake = $visualStudioCmake
    } else {
        $cmake = (Get-Command cmake.exe -ErrorAction Stop).Source
    }
    if (Test-Path $visualStudioNinja) {
        $ninja = $visualStudioNinja
    } else {
        $ninja = (Get-Command ninja.exe -ErrorAction Stop).Source
    }
    $cmakeArguments = @('-S', $projectRoot, '-B', $cmakeBuildDirectory, '-G', 'Ninja', "-DCMAKE_BUILD_TYPE=$Configuration", "-DCMAKE_MAKE_PROGRAM=$ninja")
    & $cmake @cmakeArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Application preview-plugin CMake configure failed with exit code $LASTEXITCODE."
    }
    & $cmake '--build' $cmakeBuildDirectory '--target' $config.Preview.Target '--' '-j' '2'
    if ($LASTEXITCODE -ne 0) {
        throw "Application preview plugin $Configuration build failed with exit code $LASTEXITCODE."
    }
    if (-not (Test-Path $plugin)) {
        throw "Application preview plugin was not produced: $plugin"
    }

    Write-Host "==> Rebuilding AndroidAppPreviewer $Configuration x64"
    & $msBuild $projectFile '/t:Rebuild' "/p:Configuration=$Configuration" '/p:Platform=x64' "/bl:$binaryLogPath;ProjectImports=Embed"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "==> MSBuild binary log: $binaryLogPath"
        throw "AndroidAppPreviewer $Configuration build failed with exit code $LASTEXITCODE."
    }

    Write-Host "==> MSBuild binary log: $binaryLogPath"
    if (-not (Test-Path $previewer)) {
        throw "AndroidAppPreviewer executable was not produced: $previewer"
    }

    if ($BuildOnly) {
        Write-Host "Previewer and plugin ready: $plugin"
        return
    }

    Write-Host "==> Starting $previewer"
    Start-Process -FilePath $previewer -ArgumentList '--plugin', ('"{0}"' -f $plugin) -WindowStyle Normal
} catch {
    Write-Error $_
    exit 1
}