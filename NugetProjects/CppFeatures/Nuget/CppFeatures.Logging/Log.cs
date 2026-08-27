using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

namespace CppFeatures.Logging {
    [Flags]
    public enum InitFlags {
        None = 0x00,
        Truncate = 0x01,
        AppendNewSessionMsg = 0x02,
        CreateInPackageFolder = 0x04,
        EnableLogToStdout = 0x08,
        RedirectRawTimeLogToStdout = 0x10,
        DisableEOLforRawLogger = 0x20,
        CreateInExeFolderForDesktop = 0x40,
        DefaultFlags = AppendNewSessionMsg,
        CreateInAppFolder = CreateInPackageFolder | CreateInExeFolderForDesktop
    }

    public static class Log {
        public static void Init(string logFilePath, InitFlags initFlags) {
            NativeMethods.Init(logFilePath, (int)initFlags);
        }

        public static void Debug(
            string message,
            [CallerFilePath] string filePath = "",
            [CallerMemberName] string memberName = "",
            [CallerLineNumber] int lineNumber = 0
        ) {
            Write(message, 1, filePath, memberName, lineNumber);
        }

        public static void Warning(
            string message,
            [CallerFilePath] string filePath = "",
            [CallerMemberName] string memberName = "",
            [CallerLineNumber] int lineNumber = 0
        ) {
            Write(message, 3, filePath, memberName, lineNumber);
        }

        public static void Error(
            string message,
            [CallerFilePath] string filePath = "",
            [CallerMemberName] string memberName = "",
            [CallerLineNumber] int lineNumber = 0
        ) {
            Write(message, 4, filePath, memberName, lineNumber);
        }

        private static void Write(
            string message,
            int level,
            string filePath,
            string memberName,
            int lineNumber
        ) {
            NativeMethods.Log(message, filePath, memberName, lineNumber, level);

#if DEBUG
            WriteToDebugOutput(message, level, filePath, memberName, lineNumber);
#endif
        }

#if DEBUG
        // Формат повторяет основной паттерн native-логгера, чтобы записи из .NET
        // и C++ было удобно сопоставлять в одном сеансе отладки.
        private static void WriteToDebugOutput(
            string message,
            int level,
            string filePath,
            string memberName,
            int lineNumber
        ) {
            string filename = System.IO.Path.GetFileName(filePath);
            string levelMarker = GetLevelMarker(level);
            string timestamp = DateTime.Now.ToString("dd.MM.yyyy HH:mm:ss:fff", System.Globalization.CultureInfo.InvariantCulture);
            uint threadId = NativeMethods.GetCurrentThreadId();
            string nativePrefix = GetNativeMessagePrefix(levelMarker, level);
            string formattedMessage = string.IsNullOrEmpty(nativePrefix) ? message : $"{nativePrefix} {message}";
            string output = $"[{levelMarker}] [{threadId}] {timestamp} {{{filename}:{lineNumber} {memberName}}} {formattedMessage}";

            System.Diagnostics.Debug.WriteLine(output);
        }

        private static string GetLevelMarker(int level) {
            switch (level) {
                case 3:
                    return "W";
                case 4:
                    return "E";
                default:
                    return "D";
            }
        }

        private static string GetNativeMessagePrefix(string levelMarker, int level) {
            switch (level) {
                case 3:
                case 4:
                    return $"*** [{levelMarker}] =";
                default:
                    return string.Empty;
            }
        }
#endif

        private static class NativeMethods {
            [DllImport("CppFeatures.Desktop.dll", EntryPoint = "CppFeatures_LoggerInit", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
            internal static extern void Init(string logFilePath, int initFlags);

            [DllImport("CppFeatures.Desktop.dll", EntryPoint = "CppFeatures_LoggerLog", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
            internal static extern void Log(string message, string filePath, string memberName, int lineNumber, int level);

            [DllImport("kernel32.dll")]
            internal static extern uint GetCurrentThreadId();
        }
    }
}
