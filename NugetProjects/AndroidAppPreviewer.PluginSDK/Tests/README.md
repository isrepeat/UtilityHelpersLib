# Контракт расширения preview-сессии

PreviewSessionContract.cpp загружает две DLL через публичный C ABI. Первая использует ApplyScenario по умолчанию, вторая переопределяет его. Проверяются отказ базовой реализации, вызов переопределения, проверка null и уничтожение сессии.

Для второго плагина создайте экземпляр template_app с другим именем и добавьте в прикладной PreviewSession объявление:

```cpp
bool ApplyScenario(std::string_view page, std::string_view json) override;
```

Реализация в PreviewSession.cpp:

```cpp
bool PreviewSession::ApplyScenario(std::string_view page, std::string_view json) {
    return page == "MainPage" && json == "{}";
}
```

Соберите оба плагина Debug x64. В x64 Native Tools Command Prompt скомпилируйте тест с /EHsc /std:c++20 /utf-8 и каталогом build/native пакета SDK в /I. Передайте исполняемому файлу абсолютные пути к первой и второй DLL. Успех обозначается кодом возврата 0 и строкой PASS.

Дополнительно ApkUpdaterNew/Tests/PreviewTransitions.cpp проверяет навигацию, hot reload, прозрачность и пиксели ANGLE. Android ARM64-сборка приложения проверяет, что ApplicationFramework не зависит от preview SDK.