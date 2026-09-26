function Resolve-XamlCompiler {
    param(
        [Parameter(Mandatory)]
        [string]$ProjectRoot,

        [Parameter(Mandatory)]
        [string]$AndroidHost
    )

    $packageName = 'XamlRuntime'
    $packagesRoot = Join-Path $ProjectRoot "Build\Packages\$AndroidHost"
    $source = if ($env:ANDROIDAPPKIT_NUGET_SOURCE) {
        $env:ANDROIDAPPKIT_NUGET_SOURCE
    } elseif (Test-Path -LiteralPath 'C:\NugetFeed' -PathType Container) {
        'C:\NugetFeed'
    } else {
        'https://api.nuget.org/v3/index.json'
    }
    $nuget = (Get-Command nuget.exe -ErrorAction Stop).Source

    # Не закрепляем версию XamlRuntime здесь: как и CMake-модуль пакета,
    # при каждом запуске берём последнюю версию из выбранного NuGet feed-а.
    & $nuget install $packageName -Source $source -OutputDirectory $packagesRoot -NonInteractive | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "XamlRuntime restore failed with exit code $LASTEXITCODE."
    }

    # NuGet распаковывает каждую версию в каталог XamlRuntime.<версия>.
    # Сортировка NATURAL корректно ставит 1.0.23 выше 1.0.9.
    $candidates = Get-ChildItem -LiteralPath $packagesRoot -Directory -Filter "$packageName.*" |
        Sort-Object @{ Expression = {
                [version]$_.Name.Substring($packageName.Length + 1)
            }; Descending = $true }
    foreach ($candidate in $candidates) {
        $compiler = Join-Path $candidate.FullName 'tools\win-x64\XamlCompiler.exe'
        if (Test-Path -LiteralPath $compiler -PathType Leaf) {
            return $compiler
        }
    }

    throw "NuGet package $packageName did not provide tools\\win-x64\\XamlCompiler.exe in $packagesRoot."
}