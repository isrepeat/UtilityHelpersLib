[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$packageRoot = Split-Path -Parent $PSScriptRoot
Import-Module (Join-Path $packageRoot 'tools\Modules\Module.AndroidBuildTools\Module.AndroidBuildTools.psm1') -Force
$fixture = Join-Path ([IO.Path]::GetTempPath()) ('AndroidBuildTools-preview-' + [Guid]::NewGuid().ToString('N'))
$feed = Join-Path $fixture 'feed'
$packages = Join-Path $fixture 'packages'
New-Item -ItemType Directory -Path $feed -Force | Out-Null
try {
    foreach ($version in @('1.0.9', '1.0.25', '1.0.25.9')) {
        New-Item -ItemType File -Path (Join-Path $feed "XamlRuntime.$version.nupkg") | Out-Null
        $directory = Join-Path $packages "XamlRuntime.$version"
        New-Item -ItemType Directory -Path (Join-Path $directory 'tools\win-x64') -Force | Out-Null
        New-Item -ItemType Directory -Path (Join-Path $directory 'build\native\cmake') -Force | Out-Null
        New-Item -ItemType File -Path (Join-Path $directory 'tools\win-x64\XamlCompiler.exe') | Out-Null
        New-Item -ItemType File -Path (Join-Path $directory 'build\native\cmake\XamlRuntimeConfig.cmake') | Out-Null
    }
    New-Item -ItemType File -Path (Join-Path $feed 'XamlRuntime.invalid.nupkg') | Out-Null
    # Убираем NuGet из PATH: быстрый путь должен обходиться без него.
    $originalPath = $env:PATH
    try {
        $env:PATH = $env:SystemRoot
        $compiler = Resolve-XamlCompiler -PackagesRoot $packages -Source $feed
        $expected = Join-Path $packages 'XamlRuntime.1.0.25.9\tools\win-x64\XamlCompiler.exe'
        if ($compiler -ne $expected) { throw "Expected $expected, got $compiler" }
        New-Item -ItemType File -Path (Join-Path $feed 'XamlRuntime.1.0.26.nupkg') | Out-Null
        $restoreRequired = $false
        try { Resolve-XamlCompiler -PackagesRoot $packages -Source $feed | Out-Null } catch {
            if ($_.Exception.Message -notmatch 'nuget.exe') { throw }
            $restoreRequired = $true
        }
        if (-not $restoreRequired) { throw 'A newer missing package must require restore.' }
    } finally { $env:PATH = $originalPath }
    Write-Host 'Preview input checks passed.'
} finally {
    $resolvedFixture = [IO.Path]::GetFullPath($fixture)
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    if (-not $resolvedFixture.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Unexpected fixture path: $resolvedFixture"
    }
    Remove-Item -LiteralPath $resolvedFixture -Recurse -Force
}