# MobileClock XAML Previewer

Desktop-прототип для интерактивного просмотра XAML-подобной разметки MobileClock. Решение для Visual Studio находится в `XamlPreviewer.sln`.

Решение содержит C#-оболочку `XamlPreviewer` и Windows bridge `XamlRuntime.NativeBridge`. Bridge скрыто собирает и линкует общую C++-библиотеку `XamlRuntime`; она не показывается в Solution Explorer. Собирать решение нужно в конфигурации `x64`, после чего previewer запускается обычным способом из Visual Studio.

Запуск уже собранной Debug-конфигурации из корня репозитория:

```powershell
Tools/XamlPreviewer/bin/Debug/net8.0-windows/XamlPreviewer.exe
```

При запуске автоматически открывается `Native/UI/MainPage.xaml`. Левая панель редактирует разметку, средняя показывает результат. JSON-редактор сценариев временно скрыт, но сценарии по-прежнему доступны в верхнем списке. Изменения отображаются с задержкой 250 мс. Кнопка «Сохранить» перезаписывает открытый XAML-файл.

Preview использует логический viewport `720×1280` и шрифт приложения `Roboto-Regular.ttf`. Parsing тестовых binding-данных остаётся частью редактора, но типы элементов, атрибуты, layout, clipping, chrome, `ToggleSwitch` и встроенные SVG обрабатывает общая библиотека `XamlRuntime`. Она выдаёт только платформонезависимые команды рисования; WPF исполняет их через `DrawingContext`, а Android — через OpenGL ES. Поддерживаются `Page`, `StackPanel`, `Grid`, `TextBlock`, `Border`, `Button`, `ToggleSwitch`, `ScrollViewer`, `Image`, `SvgImage`, `ListView`, простые `{Binding Property}` и `ListView.ItemTemplate`.