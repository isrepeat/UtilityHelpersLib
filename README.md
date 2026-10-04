# UtilityHelpersLib

Общий репозиторий библиотек, NuGet-пакетов, CMake-компонентов и вспомогательных
инструментов для проектов isrepeat.

## Содержание

| Компонент | Назначение | Документация |
| --- | --- | --- |
| AndroidBuildTools | PowerShell, CMake, Gradle conventions и шаблон native Android-приложения | [README](NugetProjects/AndroidBuildTools/README.md), [flow и контракт проекта](NugetProjects/AndroidBuildTools/BUILD-PIPELINE.md) |
| CppFeatures | Общие C++-возможности и вспомогательный код | [README](NugetProjects/CppFeatures/README.md) |
| AndroidCoreSdk | Android Core SDK package | [README](PackageProjects/Android/AndroidCoreSdk/README.md) |

## AndroidBuildTools

AndroidBuildTools — точка входа для нового native Android-проекта. В его
[README](NugetProjects/AndroidBuildTools/README.md) приведён bootstrap без
существующего проекта. Полный документ
[BUILD-PIPELINE.md](NugetProjects/AndroidBuildTools/BUILD-PIPELINE.md) описывает
контракт `android-build.psd1`, восстановление NuGet-пакетов, XAML, CMake, Gradle
plugins, PowerShell-модуль, подпись и диагностику сборки.

Исходники AndroidBuildTools находятся в
`NugetProjects/AndroidBuildTools`. Изменения публикуются новой версией NuGet;
потребители закрепляют её через `BuildToolsVersion` в собственном
`android-build.psd1`.