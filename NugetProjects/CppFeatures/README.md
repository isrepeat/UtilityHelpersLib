# CppFeatures: архитектура универсального NuGet-пакета

`CppFeatures` — единый NuGet-пакет, который содержит нативную C++-библиотеку,
C++/CX Windows Runtime Component, C++/WinRT-компонент и управляемую C#-проекцию.
Такое разделение позволяет переиспользовать одну реализацию в Desktop, UWP и
современных .NET/WinUI-приложениях, не переписывая весь API отдельно для каждой
платформы.

## Состав решения

### `CppFeatures.Shared`

Shared Items Project (`.vcxitems`) с общими заголовками и исходниками API.
Самостоятельного бинарного файла не создаёт. Импортируется в нативные проекты,
которые компилируют общий код со своими platform-specific настройками.

Основные правила:

- публичный нативный API находится в shared-заголовках;
- platform-specific код должен быть защищён compile-time условиями;
- namespace вспомогательного кода задаётся через `NUGET_HELPERS_NS` и
  `HELPERS_NS`, чтобы избежать конфликтов с другими копиями Helpers;
- shared-проект не должен напрямую зависеть от выходного каталога конкретной
  платформы.

### `CppFeatures.Desktop`

Обычная нативная DLL. Компилирует `CppFeatures.Shared` и необходимые shared-
части `UtilityHelpersLib` для Desktop Win32.

Выходные файлы:

- `CppFeatures.Desktop.dll` — реализация;
- `CppFeatures.Desktop.lib` — import library для нативных потребителей и
  проектов-обёрток.

Важные свойства проекта:

- `ConfigurationType=DynamicLibrary`;
- `PlatformToolset=v145` для Visual Studio 2026;
- `WindowsTargetPlatformVersion=10.0` — использовать установленный Windows SDK;
- одинаковые `Configuration` и `Platform` у библиотеки и её нативных
  потребителей;
- `LanguageStandard=stdcpp20`.

### `CppFeatures.Cx.WRC`

C++/CX Windows Runtime Component для UWP-совместимого ABI. Проект предоставляет
WinRT API (`CppFeatures.Cx.winmd`), а реализацию операций делегирует Desktop DLL.
Это позволяет держать сложную логику в одной нативной реализации.

Обязательные настройки:

- `Keyword=WindowsRuntimeComponent`;
- `AppContainerApplication=true`;
- `DesktopCompatible=true` (`Windows Desktop Compatible = Yes`);
- `ConfigurationType=DynamicLibrary`;
- `PlatformToolset=v145`;
- `WindowsTargetPlatformVersion=10.0`;
- `LanguageStandard=stdcpp17` или новее;
- `ProjectReference` на `CppFeatures.Desktop`: он задаёт и порядок сборки, и
  автоматическую линковку с `CppFeatures.Desktop.lib`.

Выходные файлы: `CppFeatures.Cx.dll`, `CppFeatures.Cx.winmd` и
`CppFeatures.Cx.pri`.

### `CppFeatures.WinRt.WRC`

C++/WinRT Windows Runtime Component. Его IDL описывает дополнительный WinRT API,
а `ProjectReference` на `CppFeatures.Cx.WRC` позволяет C++/WinRT и CsWinRT видеть
метаданные Cx-компонента. В результате C#-проекция включает как
`CppFeatures.WinRt`, так и существующий `CppFeatures.Cx` API.

Обязательные настройки:

- `ConfigurationType=DynamicLibrary`;
- `DesktopCompatible=true`;
- `PlatformToolset=v145`;
- `WindowsTargetPlatformVersion=10.0`;
- `ProjectReference` на `CppFeatures.Cx.WRC`;
- native platform mapping через `CppFeaturesNativePlatform` для цепочки
  генерации проекции (по умолчанию `x64`);
- NuGet build-зависимости из `packages.config`: CppWinRT, Windows SDK
  BuildTools, Windows App SDK и WIL.

Проект использует `Microsoft.Windows.CppWinRT 3.0.260715.1`, совместимый с
актуальным компилятором MSVC из Visual Studio 2026. Временный флаг
`_SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS`, требовавшийся старой
версии CppWinRT, больше не используется.

Выходные файлы: `CppFeatures.WinRt.WRC.dll`, `CppFeatures.WinRt.winmd` и
`CppFeatures.WinRt.WRC.pri`.

### WinAPI из C++/CX/UWP

Начиная с версии `1.0.5`, пакет содержит API, ранее находившийся в
`HelpersWinApi.Cx`. Публичный заголовок:

```cpp
#include <CppFeatures/Cx/WinApi.h>

RECT rect{};
const HWND desktop = CppFeatures::Cx::WinApi::GetDesktopWindow();
CppFeatures::Cx::WinApi::GetWindowRect(desktop, &rect);
```

Вызов проходит через `CppFeatures.Cx.dll` в `CppFeatures.Desktop.dll`, где и
выполняется настоящий WinAPI. Благодаря этому UWP/C++/CX-код не вызывает
desktop-only функцию напрямую. Отдельные `HelpersWinApi` и `HelpersWinApi.Cx`
для этих функций больше не требуются.

### `CppFeatures.WinRt.Projection`

SDK-style C#-проект с `Microsoft.Windows.CsWinRT`. Он выполняет две задачи:

1. генерирует AnyCPU C#-проекцию из `CppFeatures.Cx.winmd` и
   `CppFeatures.WinRt.winmd`;
2. собирает итоговую структуру и создаёт `CppFeatures.<version>.nupkg`.

Важные свойства:

- `TargetFramework=net6.0-windows10.0.19041.0` сохраняет заявленную минимальную
  платформу потребителя;
- `Platform=AnyCPU` относится только к managed projection assembly;
- `CsWinRTWindowsMetadata=10.0.26100.0` выбирает установленный build-time SDK;
- `CsWinRTIncludes=CppFeatures` включает обе области имён;
- `GeneratePackageOnBuild=true` только для `Release`: Debug-сборка подготавливает
  артефакты, а следующий Release-проход один раз создаёт итоговый пакет;
- `ProjectReference` на `CppFeatures.WinRt.WRC`;
- нативные WinMD берутся из ветки `CppFeaturesNativePlatform`, хотя выход
  проекции находится в `AnyCPU`.

## Граф зависимостей

```text
CppFeatures.Shared
        |
        v
CppFeatures.Desktop
        |
        v
CppFeatures.Cx.WRC
        |
        v
CppFeatures.WinRt.WRC
        |
        v
CppFeatures.WinRt.Projection
        |
        v
CppFeatures.<version>.nupkg
```

`CppFeatures.WinRt.Projection` также передаёт CsWinRT метаданные обоих WRC,
поэтому публичный Cx API появляется в итоговой C#-проекции без повторной ручной
реализации классов в стиле C++/WinRT.

## Правильный порядок сборки

Канонический способ — собрать `UtilityHelpersLib.Nugets.sln` с нужными
конфигурацией и платформой. Решение собирает:

1. `CppFeatures.Desktop` для нужных конфигураций и архитектур;
2. `CppFeatures.WinRt.WRC` (через project references также собираются
   `CppFeatures.Cx.WRC` и `CppFeatures.Desktop`);
3. `CppFeatures.WinRt.Projection` с `Release|x64`.

Для проекции параметр скрипта должен оставаться `x64`. Managed DLL всё равно
создаётся в `Build\Release\AnyCPU`, но нативные DLL и WinMD должны соответствовать
выбранной архитектуре.

Для отдельной x86-сборки указывается платформа Visual Studio `Win32` (а не
`x86`) и тот же выбор передаётся в цепочку зависимостей:

```text
/p:Platform=Win32 /p:CppFeaturesNativePlatform=Win32
```

После этого вариант упаковки задаётся как `Debug\x86;Release\x86`: это уже
архитектура NuGet RID, поэтому здесь используется `x86`, а не `Win32`.

Эквивалентная проверочная команда MSBuild для Debug x64:

```bat
msbuild UtilityHelpersLib.Nugets.sln ^
  /t:"NugetProjects\CppFeatures\Nuget\CppFeatures_WinRt_Projection" ^
  /p:Configuration=Debug /p:Platform=x64
```

Точки в имени solution target заменяются символами `_`; это автоматически
делает `BuildProject.cmd`.

Не рекомендуется собирать `CppFeatures.WinRt.Projection.csproj` напрямую с
`Platform=AnyCPU`: обычный MSBuild может передать `AnyCPU` транзитивным C++-
проектам. Собирайте projection target через `UtilityHelpersLib.Nugets.sln`.

В Visual Studio достаточно выбрать нужные `Configuration`/`Platform`, затем
вызвать **Build** или **Rebuild** у `CppFeatures.WinRt.Projection`. Вся цепочка
native-зависимостей описана через `ProjectReference`, поэтому вручную задавать
`Project Dependencies` в свойствах solution больше не нужно. Отдельные шаги в
скрипте сохранены для явной матричной сборки всех конфигураций и архитектур.

## Структура итогового NuGet

Набор нативных вариантов задаётся allow-list свойством
`CppFeaturesNugetBuildVariants` в формате `Configuration\RID-architecture`. По
умолчанию:

```xml
<CppFeaturesNugetBuildVariants>
  Debug\x64;Release\x64
</CppFeaturesNugetBuildVariants>
```

Его можно переопределить при вызове MSBuild, например для пакета только с
Release-бинарниками:

```text
/p:CppFeaturesNugetBuildVariants="Release\x64"
```

Упаковщик выбирает только перечисленные каталоги и не подхватывает устаревшие
артефакты других платформ из `!NUGET_TMP`.

Проект `CppFeatures.WinRt.Projection.targets` формирует примерно такую структуру:

```text
lib/
  net6.0-windows10.0.19041.0/
    CppFeatures.WinRt.Projection.dll
  uap10.0/
    CppFeatures.Cx.winmd
build/
  native/
  uap10.0/
  net6.0-windows10.0.19041.0/
    CppFeatures.targets
    include/CppFeatures/...
runtimes/
  win10-x64/native/Debug|Release/
    CppFeatures.Desktop.dll
    CppFeatures.Desktop.lib
    CppFeatures.Cx.dll
    CppFeatures.Cx.pri
    CppFeatures.WinRt.WRC.dll
    CppFeatures.WinRt.WRC.pri
```

Для поддержки `x86` или `ARM64` сначала должны быть собраны все нативные проекты
для этой архитектуры, а их файлы должны попасть в соответствующий RID-каталог
(`win10-x86`, `win10-arm64`). Для x86 используйте `Win32` как платформу Visual
Studio и `CppFeaturesNativePlatform=Win32`, а в allow-list укажите `x86`.
Упаковщик берёт только варианты из
`CppFeaturesNugetBuildVariants`, поэтому старые файлы из `!NUGET_TMP` не
смешиваются с текущим пакетом.

## Добавление нового универсального NuGet

Рекомендуемый шаблон:

1. создать `<Name>.Shared` с общим нативным API;
2. создать `<Name>.Desktop`, импортировать shared items и экспортировать DLL/lib;
3. создать `<Name>.Cx.WRC`, включить `DesktopCompatible`, сформировать WinMD и
   связать его с Desktop import library;
4. создать `<Name>.WinRt.WRC`, описать дополнительный API в IDL и добавить
   `ProjectReference` на Cx WRC;
5. создать `<Name>.WinRt.Projection` (AnyCPU), добавить CsWinRT и
   `ProjectReference` на WinRt WRC;
6. добавить `.targets` для подготовки `lib`, `build`, `runtimes` и include-файлов;
7. добавить `.nuspec`, перечислить поддерживаемые TFM и положить README в пакет;
8. настроить mappings в `.sln`: managed projection — AnyCPU, native projects —
   конкретная архитектура;
9. добавить build-скрипт с явным порядком Desktop → WinRt WRC → Projection;
10. проверить пакет на чистой машине или с пустым глобальным NuGet-кэшем.

Версия в `.nuspec`, имена DLL/WinMD, `RootNamespace`, пути в `.targets` и ID
пакета должны изменяться согласованно.
