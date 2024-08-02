using net.r_eg.MvsSln;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
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
    enum SlnType
    {
        Input, Output
    }

    public partial class MainWindow : Window
    {
        private Solution? inputSln;
        private Solution? outputSln;

        private IList<Project> selectedProjects = new List<Project>();

        public MainWindow()
        {
            InitializeComponent();
            UpdateUI();
        }

        private void UpdateUI()
        {
            bool inputSet = inputSln != null;
            bool outputSet = outputSln != null;

            BtOpenSourceSolutionFolder.IsEnabled = inputSet;
            BtOpenDestinationSolutionFolder.IsEnabled = outputSet;

            SetCanInject(inputSet && outputSet && selectedProjects.Count > 0);

            TbSearch.IsEnabled = inputSet;
            CbSearchIgnoreCase.IsEnabled = inputSet;
            CbSearchUseRegex.IsEnabled = inputSet;
        }

        private void SetCanInject(bool canInject)
        {
            BtInjectProjects.IsEnabled = canInject;
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

        private string? RequestSaveSolution()
        {
            var dialog = new Microsoft.Win32.SaveFileDialog();
            dialog.Filter = "Visual Studio Solution (.sln)|*.sln";

            bool? result = dialog.ShowDialog();
            if (result == true)
            {
                return dialog.FileName;
            }

            return null;
        }

        private void OpenSln(string path, SlnType type)
        {
            var solution = new Solution(path);

            switch(type)
            {
                case SlnType.Input:
                    inputSln = solution;
                    TbInputSlnPath.Text = path;
                    DgInputSlnProjects.ItemsSource = solution.Projects;
                    break;

                case SlnType.Output:
                    outputSln = solution;
                    TbOutputSlnPath.Text = path;
                    break;

                default:
                    break;
            }

            UpdateUI();
        }

        private void OnOpenSlnClicked(SlnType type)
        {
            var solutionPath = RequestOpenSolution();
            if (solutionPath != null)
            {
                try
                {
                    OpenSln(solutionPath, type);
                }
                catch (Exception ex)
                {
                    Dialogs.Error(ex.Message);
                }
            }

        }

        private void OpenInputSln_Click(object sender, RoutedEventArgs e)
        {
            OnOpenSlnClicked(SlnType.Input);
        }

        private void BtOpenOutputSln_Click(object sender, RoutedEventArgs e)
        {
            OnOpenSlnClicked(SlnType.Output);
        }

        private void DgInputSlnProjects_SelectedCellsChanged(object sender, SelectedCellsChangedEventArgs e)
        {
            foreach (var cell in e.AddedCells)
            {
                if (cell.Item is Project project)
                {
                    if (!selectedProjects.Contains(project))
                        selectedProjects.Add(project);
                }
            }

            foreach (var cell in e.RemovedCells)
            {
                if (cell.Item is Project project)
                {
                    selectedProjects.Remove(project);
                }
            }

            BtInjectProjects.Content = $"Inject {selectedProjects.Count} projects";
            UpdateUI();
        }

        private async void BtInjectProjects_Click(object sender, RoutedEventArgs e)
        {
            if (inputSln == null)
            {
                Dialogs.Error("No input solution is open.");
                return;
            }

            if (outputSln == null)
            {
                Dialogs.Error("No output solution is open.");
                return;
            }

            if (selectedProjects.Count == 0)
            {
                Dialogs.Error("No projects are selected for injection.");
                return;
            }

            foreach (var project in selectedProjects)
            {
                if (!outputSln.IsProjectInSlnDir(project))
                {
                    Dialogs.Warning($"Project {project.Name} is not located in the output solution directory.");
                }

                outputSln.AddProject(project);
            }

            var savePath = RequestSaveSolution();
            if (savePath != null)
            {
                SetCanInject(false);
                var options = MakeSlnSaveOptions();
                await outputSln.Save(savePath, options);
                SetCanInject(true);
            }
        }

        private Solution.SaveOptions MakeSlnSaveOptions()
        {
            var options = Solution.SaveOptions.None;

            if (CbRegenerateDestinationSlnGuids.IsChecked.GetValueOrDefault(false))
            {
                options |= Solution.SaveOptions.RegenerateDestinationGuids;
            }

            return options;
        }

        private void OpenFolder(string path)
        {
            Process.Start("explorer.exe", path);
        }

        private void BtOpenSourceSolutionFolder_Click(object sender, RoutedEventArgs e)
        {
            if (inputSln != null)
            {
                OpenFolder(inputSln.Props.SolutionDir);
            }
        }

        private void BtOpenDestinationSolutionFolder_Click(object sender, RoutedEventArgs e)
        {
            if (outputSln != null)
            {
                OpenFolder(outputSln.Props.SolutionDir);
            }
        }

        private void TbSearch_TextChanged(object sender, TextChangedEventArgs e)
        {
            FilterInputSlnProjects();
        }

        private void CbSearchUseRegex_Click(object sender, RoutedEventArgs e)
        {
            FilterInputSlnProjects();
        }

        private void CbSearchIgnoreCase_Click(object sender, RoutedEventArgs e)
        {
            FilterInputSlnProjects();
        }

        void FilterInputSlnProjects()
        {
            FilterInputSlnProjects(
                TbSearch.Text,
                CbSearchUseRegex.IsChecked.GetValueOrDefault(false),
                CbSearchIgnoreCase.IsChecked.GetValueOrDefault(false)
            );
        }

        void FilterInputSlnProjects(string needle, bool useRegex, bool ignoreCase)
        {
            if (inputSln == null)
            {
                return;
            }

            if (TbSearch.Text.Count() == 0)
            {
                if (DgInputSlnProjects.ItemsSource != inputSln.Projects)
                {
                    DgInputSlnProjects.ItemsSource = inputSln.Projects;
                }

                return;
            }

            IEnumerable<Project> searchResults;

            if (useRegex)
            {
                var options = RegexOptions.None;
                if (ignoreCase)
                {
                    options |= RegexOptions.IgnoreCase;
                }

                try
                {
                    var regex = new Regex(TbSearch.Text, options);
                    searchResults = inputSln.Projects.Where(project => regex.IsMatch(project.Name));
                }
                catch (Exception ex)
                {
                    return;
                }
            }
            else
            {
                searchResults = inputSln.Projects.Where(project =>
                {
                    var name = project.Name;

                    if (ignoreCase)
                    {
                        name = name.ToLower();
                        needle = needle.ToLower();
                    }

                    return name.Contains(needle);
                });
            }

            DgInputSlnProjects.ItemsSource = searchResults;
        }
    }
}
