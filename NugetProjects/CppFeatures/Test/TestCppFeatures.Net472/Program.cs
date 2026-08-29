using CppFeatures.Logging;

internal static class Program {
    private static int Main() {
        var initFlags =
            InitFlags.Truncate |
            InitFlags.CreateInExeFolderForDesktop;

        Log.Init("TestCppFeatures.Net472.log", initFlags);

        Log.Debug("Проверка caller info из net472-проекта.");
        Log.Warning("Проверка уровня Warning.");
        Log.Error("Проверка уровня Error.");
        return 0;
    }
}
