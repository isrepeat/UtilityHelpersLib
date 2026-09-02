# MobileClock XAML Previewer

Desktop-прототип для интерактивного просмотра XAML-подобной разметки MobileClock. Решение для Visual Studio находится в `XamlPreviewer.sln`.

Решение содержит C#-оболочку `XamlPreviewer` и Windows bridge `XamlRuntime.NativeBridge`. Bridge скрыто собирает и линкует общую C++-библиотеку `XamlRuntime`; она не показывается в Solution Explorer. Собирать решение нужно в конфигурации `x64`, после чего previewer запускается обычным способом из Visual Studio.

Запуск уже собранной Debug-конфигурации из корня репозитория:

```powershell
Tools/XamlPreviewer/bin/Debug/net8.0-windows/XamlPreviewer.exe
```

При первом запуске previewer создаёт `C:\WORK\TEST\XamlPreviewer\previewer.settings.json` и `scenarios.json` из debug-defaults, заданных в коде. Settings JSON хранит каталоги XAML и ресурсов, путь к сценариям, а также последнюю открытую страницу и сценарий. Левая панель переключается между XAML, сценариями и настройками; «Сохранить» записывает текущий документ. Изменения отображаются с задержкой 250 мс.

Preview использует логический viewport `720×1280` и шрифт приложения `Roboto-Regular.ttf`. `SvgImage` загружается через общий нативный OpenGL-рендерер с указанным `tint`; при недоступном файле отображается плашка `SVG`. Сценарии группируются по имени страницы без расширения. Поддерживаются `Page`, `StackPanel`, `Grid`, `TextBlock`, `Border`, `Button`, `ToggleSwitch`, `ScrollViewer`, `Image`, `SvgImage`, `ListView`, простые `{Binding Property}` и `ListView.ItemTemplate`.