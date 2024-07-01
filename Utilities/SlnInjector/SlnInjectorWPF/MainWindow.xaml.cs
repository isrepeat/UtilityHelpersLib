using net.r_eg.MvsSln;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace SlnInjectorWPF
{
    public partial class MainWindow : Window
    {
        private Solution? inputSln;

        public MainWindow()
        {
            InitializeComponent();
        }

        private string? RequestOpenSolution()
        {
            var dialog = new Microsoft.Win32.OpenFileDialog();
            dialog.Filter = "Visual Studio Solution (.sln)|*.sln";

            bool? result = dialog.ShowDialog();
            if (result == true)
            {
                return dialog.FileName;
            }

            return null;
        }

        private void OpenInputSln(string path)
        {
            inputSln = new Solution(path);
            TbInputSlnPath.Text = path;
            DgInputSlnProjects.ItemsSource = inputSln.Projects;
        }

        private void OpenInputSln_Click(object sender, RoutedEventArgs e)
        {
            var solutionPath = RequestOpenSolution();
            if (solutionPath != null)
            {
                try
                {
                    OpenInputSln(solutionPath);
                }
                catch (Exception ex)
                {
                    Dialogs.Error(ex.Message);
                }
            }
        }

        private void DgInputSlnProjects_Selected(object sender, RoutedEventArgs e)
        {

        }

        private void DgInputSlnProjects_SelectedCellsChanged(object sender, SelectedCellsChangedEventArgs e)
        {

        }
    }
}
