using Microsoft.Build.Locator;
using System.Windows;

namespace SlnInjectorWPF
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        public App()
        {
            // Required for MvsSln to be able to resolve project Import sections
            // with references to Microsoft.Cpp.Default.props, Microsoft.Cpp.targets, etc.

            MSBuildLocator.AllowQueryAllRuntimeVersions = true;
            MSBuildLocator.RegisterDefaults();

            if (!MSBuildLocator.IsRegistered)
            {
                Dialogs.Error("Couldn't locate MSBuild assemblies.");
            }
        }
    }
}
