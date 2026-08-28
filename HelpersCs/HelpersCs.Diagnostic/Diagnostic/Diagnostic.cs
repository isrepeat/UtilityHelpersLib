#if NETSTANDARD2_0
using System;
using System.Runtime.CompilerServices;

namespace Helpers {
    // Анализатор собирается под netstandard2.0 и не загружает desktop-адаптер.
    // Ему достаточно совместимого API без фактической записи журнала.
    public class Diagnostic {
        private static readonly Diagnostic _instance = new Diagnostic();
        public static Diagnostic Instance => _instance;
        public static Logger Logger { get; } = new Logger();
    }

    public struct CallerInfo {
        public string FilePath { get; }
        public string CallerName { get; }
        public string MemberName { get; }
        public int LineNumber { get; }

        public CallerInfo(string filePath, string callerName, string memberName, int lineNumber) {
            FilePath = filePath;
            CallerName = callerName;
            MemberName = memberName;
            LineNumber = lineNumber;
        }
    }

    public class Logger {
        public bool IsLoggingEnabled { get; set; } = true;
        public void EnableFileLogging(string logFilePath, bool appendNewSession = false) { }
        public void LogDebug(string logMessage, string caller = "", [CallerFilePath] string filePath = "", [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0) { }
        public void LogWarning(string logMessage, string caller = "", [CallerFilePath] string filePath = "", [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0) { }
        public void LogError(string logMessage, string caller = "", [CallerFilePath] string filePath = "", [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0) { }
        public void LogParam(string logMessage, string caller = "", [CallerFilePath] string filePath = "", [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0) { }
        public Releaser LogFunctionScope(string logMessage, string caller = "", [CallerFilePath] string filePath = "", [CallerMemberName] string memberName = "", [CallerLineNumber] int lineNumber = 0) => Releaser.Noop;

        public sealed class Releaser : IDisposable {
            internal static readonly Releaser Noop = new Releaser();
            public void Dispose() { }
        }
    }
}
#else
using System;
using System.Runtime.CompilerServices;

namespace Helpers {
    public class Diagnostic {
        private static readonly Diagnostic _instance = new Diagnostic();
        public static Diagnostic Instance => _instance;
        public static Logger Logger { get; } = new Logger();
    }

    public struct CallerInfo {
        public string FilePath { get; }
        public string CallerName { get; }
        public string MemberName { get; }
        public int LineNumber { get; }

        public CallerInfo(string filePath, string callerName, string memberName, int lineNumber) {
            FilePath = filePath;
            CallerName = callerName;
            MemberName = memberName;
            LineNumber = lineNumber;
        }
    }

    // Сохраняет прежний API Helpers.Diagnostic, направляя записи в NuGet CppFeatures.
    public class Logger {
        private const string LogParamPrefix = "  * ";
        private volatile bool _isLoggingEnabled = true;

        public bool IsLoggingEnabled {
            get => _isLoggingEnabled;
            set => _isLoggingEnabled = value;
        }

        public void EnableFileLogging(string logFilePath, bool appendNewSession = false) {
            if (string.IsNullOrWhiteSpace(logFilePath)) {
                throw new ArgumentException("Путь к файлу журнала не задан.", nameof(logFilePath));
            }

            var initializationFlags = appendNewSession
                ? CppFeatures.Logging.InitFlags.AppendNewSessionMsg
                : CppFeatures.Logging.InitFlags.Truncate;
            CppFeatures.Logging.Log.Init(logFilePath, initializationFlags);
        }

        public void LogDebug(
            string logMessage,
            string caller = "",
            [CallerFilePath] string filePath = "",
            [CallerMemberName] string memberName = "",
            [CallerLineNumber] int lineNumber = 0
        ) {
            if (!this.IsLoggingEnabled) {
                return;
            }

            CppFeatures.Logging.Log.Debug(FormatMessage(logMessage, caller), filePath, memberName, lineNumber);
        }

        public void LogWarning(
            string logMessage,
            string caller = "",
            [CallerFilePath] string filePath = "",
            [CallerMemberName] string memberName = "",
            [CallerLineNumber] int lineNumber = 0
        ) {
            if (!this.IsLoggingEnabled) {
                return;
            }

            CppFeatures.Logging.Log.Warning(FormatMessage(logMessage, caller), filePath, memberName, lineNumber);
        }

        public void LogError(
            string logMessage,
            string caller = "",
            [CallerFilePath] string filePath = "",
            [CallerMemberName] string memberName = "",
            [CallerLineNumber] int lineNumber = 0
        ) {
            if (!this.IsLoggingEnabled) {
                return;
            }

            CppFeatures.Logging.Log.Error(FormatMessage(logMessage, caller), filePath, memberName, lineNumber);
        }

        public void LogParam(
            string logMessage,
            string caller = "",
            [CallerFilePath] string filePath = "",
            [CallerMemberName] string memberName = "",
            [CallerLineNumber] int lineNumber = 0
        ) {
            if (!this.IsLoggingEnabled) {
                return;
            }

            CppFeatures.Logging.Log.Debug(FormatMessage(LogParamPrefix + logMessage, caller), filePath, memberName, lineNumber);
        }

        public Releaser LogFunctionScope(
            string logMessage,
            string caller = "",
            [CallerFilePath] string filePath = "",
            [CallerMemberName] string memberName = "",
            [CallerLineNumber] int lineNumber = 0
        ) {
#if DEBUG
            if (!this.IsLoggingEnabled) {
                return Releaser.Noop;
            }

            var callerInfo = new CallerInfo(filePath, caller, memberName, lineNumber);
            CppFeatures.Logging.Log.Debug(FormatMessage(logMessage + " enter", caller), filePath, memberName, lineNumber);
            return new Releaser(logMessage, callerInfo);
#else
            return Releaser.Noop;
#endif
        }

        private static string FormatMessage(string logMessage, string caller) {
            return string.IsNullOrEmpty(caller) ? logMessage : $"{logMessage} [{caller}]";
        }

        public class Releaser : IDisposable {
            internal static readonly Releaser Noop = new Releaser();

            private readonly string _logMessage = "";
            private readonly CallerInfo _callerInfo;
            private bool _disposed;

            private Releaser() {
                _disposed = true;
            }

            internal Releaser(string logMessage, CallerInfo callerInfo) {
                _logMessage = logMessage;
                _callerInfo = callerInfo;
            }

            public void Dispose() {
                if (_disposed) {
                    return;
                }

                CppFeatures.Logging.Log.Debug(
                    FormatMessage(_logMessage + " exit", _callerInfo.CallerName),
                    _callerInfo.FilePath,
                    _callerInfo.MemberName,
                    _callerInfo.LineNumber
                );

                _disposed = true;
            }
        }
    }
}
#endif
