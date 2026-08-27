using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using System;
using System.Collections.Generic;
using System.Dynamic;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;


namespace TestCppFeatures.WinUI {
    public class Diagnostic {
        private static readonly Diagnostic _instance = new Diagnostic();
        public static Diagnostic Instance => _instance;
        public static Logger Logger { get; set; } = new Logger();
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
        public void LogDebug(
            string logMessage,
            string caller = "",
            [CallerFilePath] string filePath = "",
            [CallerMemberName] string memberName = "",
            [CallerLineNumber] int lineNumber = 0
            ) {
            var callerInfo = new CallerInfo(filePath, caller, memberName, lineNumber);
            if (String.IsNullOrEmpty(caller)) {
                CppFeatures.Cx.Logger.LogDebug($"{logMessage}", callerInfo.FilePath, callerInfo.MemberName, callerInfo.LineNumber);
            }
            else {
                CppFeatures.Cx.Logger.LogDebug($"{logMessage} [{callerInfo.CallerName}]", callerInfo.FilePath, callerInfo.MemberName, callerInfo.LineNumber);
            }
        }

        public Releaser LogFunctionScope(
            string logMessage,
            string caller = "",
            [CallerFilePath] string filePath = "",
            [CallerMemberName] string memberName = "",
            [CallerLineNumber] int lineNumber = 0
            ) {
            var callerInfo = new CallerInfo(filePath, caller, memberName, lineNumber);
            if (!String.IsNullOrEmpty(callerInfo.CallerName)) {
                CppFeatures.Cx.Logger.LogDebug($"{logMessage} enter [{callerInfo.CallerName}]", callerInfo.FilePath, callerInfo.MemberName, callerInfo.LineNumber);
            }
            else {
                CppFeatures.Cx.Logger.LogDebug($"{logMessage} enter", callerInfo.FilePath, callerInfo.MemberName, callerInfo.LineNumber);
            }
            return new Releaser(logMessage, callerInfo);
        }

        public class Releaser : IDisposable {
            public string logMessage = "";
            public CallerInfo callerInfo;
            public Releaser(string logMessage, CallerInfo callerInfo) {
                this.logMessage = logMessage;
                this.callerInfo = callerInfo;
            }

            public void Dispose() {
                if (!String.IsNullOrEmpty(this.callerInfo.CallerName)) {
                    CppFeatures.Cx.Logger.LogDebug($"{this.logMessage} exit [{this.callerInfo.CallerName}]", this.callerInfo.FilePath, this.callerInfo.MemberName, this.callerInfo.LineNumber);
                }
                else {
                    CppFeatures.Cx.Logger.LogDebug($"{this.logMessage} exit", this.callerInfo.FilePath, this.callerInfo.MemberName, this.callerInfo.LineNumber);
                }
            }
        }
    }
    enum ClientMessages {
        None,
        Connect,
        ActivatedByFile,
        ActivatedByProtocol,
        CloseRequest
    };
    public sealed partial class MainWindow : Window {
        //private CppFeatures.Cx.Channel channel;

        public MainWindow() {
            this.InitializeComponent();

            var initFlags = CppFeatures.Cx.InitFlags.DefaultFlags | CppFeatures.Cx.InitFlags.CreateInPackageFolder;
            CppFeatures.Cx.Logger.Init("TestCppFeature.WinUI.log", initFlags);
            Diagnostic.Logger.LogDebug("MainWindow hello");

            NestedFuncA();
            NestedFuncB();
        }

        private void NestedFuncA() {
            using var __logFunctionScoped = Diagnostic.Logger.LogFunctionScope("NestedFuncA()", "MainWindow");
            Diagnostic.Logger.LogDebug("NestedFuncA hello");
        }

        private void NestedFuncB() {
            using var __logFunctionScoped = Diagnostic.Logger.LogFunctionScope("NestedFuncB()");
            Diagnostic.Logger.LogDebug("NestedFuncB hello");
        }

        private void myButton_Click(object sender, RoutedEventArgs e) {
            //CppFeatures.Cx.Logger.LogDebug("hello", "MainWindows.xaml.cs", "myButton_Click", 36);

            //this.channel = new CppFeatures.Cx.Channel();
            //this.channel.Open(
            //    "\\\\.\\pipe\\testChannel",
            //    new CppFeatures.Cx.ListenHandler((int msgType, string payload) => {
            //        switch ((ClientMessages)msgType) {
            //            case ClientMessages.Connect:
            //                this.channel.Write((int)ClientMessages.CloseRequest, "bye");
            //                break;

            //            case ClientMessages.ActivatedByProtocol:
            //                break;
            //        }
            //        return true;
            //    }));
            this.myButton.Content = "Clicked";
        }
    }
}
