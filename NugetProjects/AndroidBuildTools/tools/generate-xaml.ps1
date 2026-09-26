[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$ProjectRoot,
[string]$CMakeExecutable)

$ErrorActionPreference = 'Stop'
$utf8Encoding = [System.Text.UTF8Encoding]::new($false)
[Console]::InputEncoding = $utf8Encoding
[Console]::OutputEncoding = $utf8Encoding
$OutputEncoding = $utf8Encoding

. (Join-Path $PSScriptRoot 'ProjectConfiguration.ps1')
$config = Read-AndroidBuildConfiguration $ProjectRoot
$applicationRoot = Join-Path $projectRoot $config.Application
$uiRoot = Join-Path $projectRoot $config.UI
$xamlCompilerRoot = Join-Path $projectRoot $config.Xaml.CompilerSource
$xamlCompilerBuild = Join-Path $projectRoot $config.Xaml.CompilerBuild
$xamlCompiler = Join-Path $xamlCompilerBuild 'Debug\XamlCompiler.exe'
$xamlSourceRoots = @(
    @{
        Source = Join-Path $applicationRoot 'UI'
        Generated = Join-Path $projectRoot "!Generated\$($config.Application)\Xaml"
    },
    @{
        Source = $uiRoot
        Generated = Join-Path $projectRoot "!Generated\$($config.UI)\Xaml"
    }
)
$xamlIgnoreConfigurationPath = Join-Path $applicationRoot 'UI\XamlCompilerIgnore.json'
$xamlIgnoreConfiguration = Get-Content -LiteralPath $xamlIgnoreConfigurationPath -Raw | ConvertFrom-Json
$xamlIgnoredDirectories = @($xamlIgnoreConfiguration.directories)
$xamlIgnoredFileSuffixes = @($xamlIgnoreConfiguration.fileSuffixes)

function Invoke-Checked {
    param(
        [Parameter(Mandatory)] [string]$Program,
        [Parameter(Mandatory)] [string[]]$Arguments
    )

    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $Program $($Arguments -join ' ')"
    }
}

. (Join-Path $PSScriptRoot 'Resolve-BuildTools.ps1')
$tools = Resolve-AndroidBuildTools -CMakeExecutable $CMakeExecutable
$cmake = $tools.CMake

$xamlCompilerNeedsBuild = -not (Test-Path $xamlCompiler)
if (-not $xamlCompilerNeedsBuild) {
    $xamlCompilerExecutableTime = (Get-Item -LiteralPath $xamlCompiler).LastWriteTimeUtc
    $xamlCompilerNeedsBuild = $null -ne (Get-ChildItem -LiteralPath $xamlCompilerRoot -Recurse -File | Where-Object {
        $_.LastWriteTimeUtc -gt $xamlCompilerExecutableTime
    } | Select-Object -First 1)
}

if ($xamlCompilerNeedsBuild) {
    Write-Host '==> Building XamlCompiler host tool'
    $xamlCompilerCache = Join-Path $xamlCompilerBuild 'CMakeCache.txt'
    if (Test-Path $xamlCompilerCache) {
        Invoke-Checked $cmake @('-S', $xamlCompilerRoot, '-B', $xamlCompilerBuild)
    } else {
        Invoke-Checked $cmake @('-S', $xamlCompilerRoot, '-B', $xamlCompilerBuild, '-G', $tools.Generator, '-A', 'x64', "-DCMAKE_GENERATOR_INSTANCE=$($tools.VisualStudio)")
    }
    Invoke-Checked $cmake @('--build', $xamlCompilerBuild, '--config', 'Debug')
}
if (-not (Test-Path $xamlCompiler)) {
    throw "XamlCompiler build completed but did not produce $xamlCompiler"
}

foreach ($xamlSourceRoot in $xamlSourceRoots) {
    Get-ChildItem -LiteralPath $xamlSourceRoot.Source -Filter '*.xaml' -File -Recurse | ForEach-Object {
        $relativePath = $_.FullName.Substring($xamlSourceRoot.Source.Length).TrimStart('\', '/')
        $generatedPath = Join-Path $xamlSourceRoot.Generated ($relativePath + '.cpp')
        $generatedHeaderPath = [System.IO.Path]::ChangeExtension($generatedPath, '.h')
        $generatedFiles = @($generatedPath, $generatedHeaderPath)
        $generationInputs = @($_.FullName, $xamlCompiler, $xamlIgnoreConfigurationPath)
        $requiresGeneration = $generatedFiles | Where-Object { -not (Test-Path $_) }
        if ($null -eq $requiresGeneration) {
            $oldestGeneratedFile = Get-Item -LiteralPath $generatedFiles | Sort-Object LastWriteTimeUtc | Select-Object -First 1
            $requiresGeneration = $generationInputs | Where-Object {
                (Get-Item -LiteralPath $_).LastWriteTimeUtc -gt $oldestGeneratedFile.LastWriteTimeUtc
            }
        }
        if ($null -ne $requiresGeneration) {
            $compilerArguments = @(
                $_.FullName,
                $generatedPath,
                '--xaml-namespace',
                $config.Xaml.Namespace,
                '--control-xml-prefix',
                'control',
                '--control-cpp-namespace',
                $config.Xaml.ControlNamespace,
                '--control-include-prefix',
                $config.Xaml.ControlIncludePrefix
            )
            foreach ($directory in $xamlIgnoredDirectories) {
                $compilerArguments += '--ignore-directory', $directory
            }
            foreach ($suffix in $xamlIgnoredFileSuffixes) {
                $compilerArguments += '--ignore-file-suffix', $suffix
            }
            Write-Host "==> Compiling $($_.Name) into native UI classes"
            Invoke-Checked $xamlCompiler $compilerArguments
        }
    }
}