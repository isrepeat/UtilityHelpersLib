# Проверка ColorAnimation

ColorAnimationTests.cpp сравнивает скомпилированную страницу и дерево RuntimeTreeBuilder: начало, середину и завершение Show, визуальное состояние с from="Current", интерполяцию RGBA и отказ при ошибочной разметке. Отдельно проверяет ограничение каждого opacity до композиции с родителем и альфой цвета.

Сначала скомпилируйте ColorAnimationPage.xaml через XamlCompiler в отдельный build-каталог. Затем соберите ColorAnimationTests.cpp с C++20, UTF-8, заголовками XamlRuntime и этим каталогом в include path. Для Debug используйте /MDd и библиотеки XamlRuntime.lib, Helpers.Logging.lib той же конфигурации. Передайте тесту путь к исходному ColorAnimationPage.xaml:

```text
ColorAnimationTests.exe path/to/ColorAnimationPage.xaml
```