# Общая инфраструктура приложений

Шаблон создаёт прикладные классы-наследники. Общие алгоритмы хранятся в UtilityHelpersLib и поставляются исходниками в пакетах; исправления инфраструктуры приходят с обновлением пакетов.

## AndroidApplication::Framework

Исходники находятся в AndroidBuildTools/Nuget/AndroidBuildTools.Package/build/native/AndroidBuildTools/ApplicationFramework. CMake собирает InputDispatcher в статическую библиотеку тем же toolchain, что и приложение. Базы, зависящие от типов приложения, являются шаблонными; их реализации находятся в заголовках.

- AppSessionControllerBase реализует передачу host-команд и состояние сообщения.
- ApplicationSessionBase владеет хранилищем, репозиторием, контроллером и менеджером страниц; предоставляет общий жизненный цикл.
- PageManagerBase реализует ввод, историю, переходы, анимации и preview hot reload.

ApplicationSessionTraits и PageManagerTraits связывают общие базы с конкретными типами приложения. Благодаря этому прикладной контроллер хранится с его полным типом, без срезки объекта. Порядок полей базы обеспечивает, что репозиторий и контроллер существуют дольше менеджера страниц.

PageManager приложения задаёт Routes(). Возвращаемый span и контракты маршрутов должны жить всё время работы сессии. Для ограничения навигации переопределяется CanNavigate(). Проверки конкретного бизнес-процесса не добавляются в общую базу.

При необходимости приложение расширяет свой контроллер собственными сервисами и переопределяет Update()/Render() своей ApplicationSession, вызывая базовую реализацию. Инициализация UI выполняется после завершения конструирования.

## Preview SDK

AndroidAppPreviewer.PluginSDK/build/native/AndroidAppPreviewer.PluginSDK содержит SessionAbi, C ABI, инспекцию, ANGLE, сериализацию графа и базы preview-сессии. android_app_previewer_add_runtime подключает эти исходники к DLL плагина. В приложении остаются PluginDefinition.cpp и Session/PreviewSession.

PluginDefinition реализует фабрику CreatePreviewSession и метаданные PluginInfoJson. Фабрика возвращает уникальный указатель на PreviewSessionBase; SDK владеет сессией и уничтожает её через виртуальный деструктор.

Единственный include-корень каждого пакета — build/native. Клиент подключает `<AndroidAppPreviewer.PluginSDK/Session/ApplicationPreviewSession.h>` и `<AndroidBuildTools/ApplicationFramework/ApplicationSessionBase.h>`. Папка SDK Internal содержит детали реализации; клиентские расширения используют контракты Session. CMake-модули находятся в корневой папке cmake пакета.

PreviewSession приложения наследует ApplicationPreviewSession<ApplicationSession>. Точки расширения:

- ApplyScenario(page, json): приложение проверяет страницу и разбирает собственный JSON. По умолчанию возвращается ошибка отсутствия поддержки сценариев.
- BeforeUpdate(): выполняет preview-работу перед обновлением приложения; возвращает признак изменения.
- RenderBackground(renderer): рисует подложку перед приложением.
- AfterRender(): вызывается после команд рисования приложения. Это не уведомление ОС о показе кадра на дисплее.
- OnPlaybackRateChanged(value): синхронизирует скорость прикладной симуляции.
- Application(): защищённый доступ наследника к сессии приложения.

SDK направляет запрос сценария виртуальному обработчику. Названия страниц и формат прикладного сценария не зашиты в SessionAbi. Общие Update/Render сохраняют порядок вызовов точек расширения.

## Проверка

Проверять нужно не только существующий проект, но и экземпляр шаблона с другим именем: это выявляет зависимости общих исходников от конкретного приложения. Для ApkUpdaterNew используются Debug preview, Android ARM64 native-сборка и Tests/PreviewTransitions.cpp, проверяющий C ABI и пиксели ANGLE.

Для шаблона нужны AndroidBuildTools с AndroidApplication::Framework и SDK с runtime (начиная с 1.0.63.27 и 2.0.4.8 соответственно). SDK 2.0.4.8 использует экспорт xp_get_abi, типы xp_*_abi и реализацию Internal/Abi в namespace preview_sdk::abi. После перехода с xp_get_api необходимо обновить SDK загрузчика AndroidAppPreviewer и пересобрать плагины. Структура таблиц и номера версий ABI не изменились.