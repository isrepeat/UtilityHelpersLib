[CmdletBinding()]
param([string]$FeedRoot = $env:UH_NUGET_FEED)

$ErrorActionPreference = 'Stop'
$packageRoot = $PSScriptRoot
$headerPath = Join-Path $packageRoot 'build\native\include\AndroidAppPreviewer.PluginSDK\AndroidAppPreviewerPlugin.h'
if (-not (Test-Path -LiteralPath $headerPath -PathType Leaf)) {
    throw "Plugin SDK header was not found: $headerPath"
}

if ([string]::IsNullOrWhiteSpace($FeedRoot)) {
    $FeedRoot = 'C:\NugetFeed'
}

$stagingRoot = Join-Path $packageRoot '!NUGET_STAGING'
$managedProject = Join-Path $packageRoot '..\AndroidAppPreviewer.PluginSDK.WPF\AndroidAppPreviewer.PluginSDK.WPF.csproj'
$nugetProjectsRoot = Resolve-Path (Join-Path $packageRoot '..\..\..')
Remove-Item -LiteralPath $stagingRoot -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path (Join-Path $stagingRoot 'build\native\include\AndroidAppPreviewer.PluginSDK'), (Join-Path $stagingRoot 'build\native\cmake'), (Join-Path $stagingRoot 'lib\net8.0'), $FeedRoot -Force | Out-Null
Copy-Item -LiteralPath $headerPath -Destination (Join-Path $stagingRoot 'build\native\include\AndroidAppPreviewer.PluginSDK\AndroidAppPreviewerPlugin.h')
Copy-Item -LiteralPath (Join-Path $packageRoot 'cmake\AndroidAppPreviewerPluginConfig.cmake') -Destination (Join-Path $stagingRoot 'build\native\cmake\AndroidAppPreviewerPluginConfig.cmake')
Copy-Item -LiteralPath (Join-Path $packageRoot 'AndroidAppPreviewer.PluginSDK.nuspec') -Destination (Join-Path $stagingRoot 'AndroidAppPreviewer.PluginSDK.nuspec')

& dotnet build $managedProject --configuration Release
if ($LASTEXITCODE -ne 0) {
    throw 'Managed Plugin SDK assembly build failed.'
}
$managedAssembly = Join-Path $nugetProjectsRoot '!NUGET_TMP\Build\Release\AndroidAppPreviewer.PluginSDK.WPF\AndroidAppPreviewer.PluginSDK.dll'
if (-not (Test-Path -LiteralPath $managedAssembly -PathType Leaf)) {
    throw "Managed Plugin SDK assembly was not found: $managedAssembly"
}
Copy-Item -LiteralPath $managedAssembly -Destination (Join-Path $stagingRoot 'lib\net8.0\AndroidAppPreviewer.PluginSDK.dll')

$vsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vsWhere)) {
    throw "vswhere.exe was not found: $vsWhere"
}
$msBuild = & $vsWhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if ([string]::IsNullOrWhiteSpace($msBuild)) {
    throw 'MSBuild.exe was not found.'
}
& $msBuild (Join-Path $packageRoot 'AndroidAppPreviewer.PluginSDK.Package.vcxproj') '/t:Pack' '/p:Configuration=Release' '/p:Platform=x64' "/p:PackageOutputPath=$FeedRoot"
if ($LASTEXITCODE -ne 0) {
    throw 'NuGet package creation failed.'
}