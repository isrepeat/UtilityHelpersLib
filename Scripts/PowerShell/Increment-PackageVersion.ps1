[CmdletBinding()]
param([Parameter(Mandatory)] [string]$ManifestPath)

$ErrorActionPreference = 'Stop'
# Меняем последнюю часть полной версии непосредственно в исходном nuspec.
$content = [System.IO.File]::ReadAllText($ManifestPath)
$match = [regex]::Match($content, '<version>(\d+(?:\.\d+){2,3})</version>')
if (-not $match.Success) { throw "Full package version is missing in $ManifestPath." }
$parts = $match.Groups[1].Value.Split('.')
$parts[$parts.Length - 1] = ([int]$parts[$parts.Length - 1] + 1).ToString()
$nextVersion = $parts -join '.'
$content = $content.Remove($match.Groups[1].Index, $match.Groups[1].Length).Insert($match.Groups[1].Index, $nextVersion)
[System.IO.File]::WriteAllText($ManifestPath, $content.TrimEnd(), [System.Text.UTF8Encoding]::new($false))
Write-Output $nextVersion