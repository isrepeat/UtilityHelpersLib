using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

namespace TestCppFeatures.UWP {
    enum ClientMessages {
        None,
        Connect,
        ActivatedByFile,
        ActivatedByProtocol,
        CloseRequest
    };
    public sealed partial class MainPage : Page {
        private CppFeatures.Cx.Channel channel;
        public MainPage() {
            this.InitializeComponent();

            this.channel = new CppFeatures.Cx.Channel();
            this.channel.Open(
                "\\\\.\\pipe\\Local\\testChannel",
                new CppFeatures.Cx.ListenHandler((int msgType, string payload) => {
                    switch ((ClientMessages)msgType) {
                        case ClientMessages.Connect:
                            this.channel.Write((int)ClientMessages.CloseRequest, "bye");
                            break;

                        case ClientMessages.ActivatedByProtocol:
                            break;
                    }
                    return true;
                })
            );
        }
    }
}
