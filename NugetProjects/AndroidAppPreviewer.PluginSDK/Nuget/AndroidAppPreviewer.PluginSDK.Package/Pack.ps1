[CmdletBinding()]
param([string]$FeedRoot)

$ErrorActionPreference = 'Stop'

$utf8Encoding = [System.Text.UTF8Encoding]::new($false)
[Console]::InputEncoding = $utf8Encoding
[Console]::OutputEncoding = $utf8Encoding
$OutputEncoding = $utf8Encoding
$packageRoot = $PSScriptRoot
$headerPath = Join-Path $packageRoot 'build\native\include\AndroidAppPreviewer.PluginSDK\AndroidAppPreviewerPlugin.h'
if (-not (Test-Path -LiteralPath $headerPath -PathType Leaf)) {
    throw "Plugin SDK header was not found: $headerPath"
}

$feedResolver = Join-Path $PSScriptRoot '..\..\..\..\Scripts\PowerShell\Resolve-PackagesFeed.ps1'
$FeedRoot = & $feedResolver -FeedPath $FeedRoot

$stagingRoot = Join-Path $packageRoot '!NUGET_STAGING'
$managedProject = Join-Path $packageRoot '..\AndroidAppPreviewer.PluginSDK.WPF\AndroidAppPreviewer.PluginSDK.WPF.csproj'
$nugetProjectsRoot = Resolve-Path (Join-Path $packageRoot '..\..\..')
$nuget = Get-Command nuget.exe -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty Source
if ([string]::IsNullOrWhiteSpace($nuget) -or -not (Test-Path -LiteralPath $nuget)) {
    throw 'nuget.exe was not found. Install the official NuGet CLI and make it available in PATH.'
}

function Get-NextPackageVersion([string]$ManifestPath, [string]$PackagesFeedPath) {
    # Полная версия хранится в исходниках; увеличиваем только последнюю часть.
    $content = [System.IO.File]::ReadAllText($ManifestPath)
    $match = [regex]::Match($content, '<version>(\d+(?:\.\d+){2,3})</version>')
    if (-not $match.Success) { throw "Full package version is missing in $ManifestPath." }
    $parts = $match.Groups[1].Value.Split('.')
    $parts[$parts.Length - 1] = ([int]$parts[$parts.Length - 1] + 1).ToString()
    $nextVersion = $parts -join '.'
    $content = $content.Remove($match.Groups[1].Index, $match.Groups[1].Length).Insert($match.Groups[1].Index, $nextVersion)
    [System.IO.File]::WriteAllText($ManifestPath, $content.TrimEnd(), [System.Text.UTF8Encoding]::new($false))
    return $nextVersion
}

$manifestPath = Join-Path $packageRoot 'AndroidAppPreviewer.PluginSDK.nuspec'
$packageVersion = Get-NextPackageVersion $manifestPath $FeedRoot
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

& $nuget pack (Join-Path $stagingRoot 'AndroidAppPreviewer.PluginSDK.nuspec') '-BasePath' $stagingRoot '-OutputDirectory' $FeedRoot '-Version' $packageVersion '-NoPackageAnalysis' '-NonInteractive' '-ForceEnglishOutput'
if ($LASTEXITCODE -ne 0) {
    throw 'NuGet CLI package creation failed.'
}