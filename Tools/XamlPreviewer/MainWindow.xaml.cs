using Microsoft.Win32;
using System.IO;
using System.Text.Json;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Threading;

namespace XamlPreviewer;

public partial class MainWindow : Window {
    private readonly DispatcherTimer renderTimer;
    private string? markupPath;
    private bool updatingEditors;

    public MainWindow() {
        InitializeComponent();
        this.renderTimer = new DispatcherTimer {
            Interval = TimeSpan.FromMilliseconds(250)
        };
        this.renderTimer.Tick += this.RenderTimerTick;
        this.Loaded += this.WindowLoaded;
    }

    private void WindowLoaded(object sender, RoutedEventArgs eventArgs) {
        var candidate = FindWorkspaceFile("Native", "UI", "MainPage.xaml");
        if (candidate is not null) {
            this.LoadMarkup(candidate);
        }

        var scenarioPath = FindWorkspaceFile("Tools", "XamlPreviewer", "scenarios.json");
        this.updatingEditors = true;
        this.ScenarioEditor.Text = scenarioPath is null
            ? "{\n  \"Default\": {}\n}"
            : File.ReadAllText(scenarioPath);
        this.updatingEditors = false;
        this.RefreshScenarioNames();
        this.ScheduleRender();
    }

    private void OpenButtonClick(object sender, RoutedEventArgs eventArgs) {
        var dialog = new OpenFileDialog {
            Filter = "XAML-like markup (*.xaml)|*.xaml|All files (*.*)|*.*"
        };
        if (dialog.ShowDialog(this) == true) {
            this.LoadMarkup(dialog.FileName);
        }
    }

    private void SaveButtonClick(object sender, RoutedEventArgs eventArgs) {
        if (this.markupPath is null) {
            return;
        }

        File.WriteAllText(this.markupPath, this.MarkupEditor.Text.TrimEnd());
        this.StatusText.Foreground = PreviewRenderer.ParseBrush("#8FD18B");
        this.StatusText.Text = $"Сохранено: {this.markupPath}";
    }

    private void ScenarioPickerSelectionChanged(object sender, SelectionChangedEventArgs eventArgs) {
        this.ScheduleRender();
    }

    private void EditorTextChanged(object sender, TextChangedEventArgs eventArgs) {
        if (!this.updatingEditors) {
            if (ReferenceEquals(sender, this.ScenarioEditor)) {
                this.RefreshScenarioNames();
            }

            this.ScheduleRender();
        }
    }

    private void RenderTimerTick(object? sender, EventArgs eventArgs) {
        this.renderTimer.Stop();
        try {
            using var document = JsonDocument.Parse(this.ScenarioEditor.Text);
            var scenarioName = this.ScenarioPicker.SelectedItem as string;
            var scenarios = document.RootElement;
            var data = scenarioName is not null && scenarios.TryGetProperty(scenarioName, out var selected)
                ? selected
                : scenarios;
            this.DeviceSurface.Child = PreviewRenderer.Render(this.MarkupEditor.Text, data, this.markupPath);
            this.StatusText.Foreground = PreviewRenderer.ParseBrush("#8FD18B");
            this.StatusText.Text = $"Предпросмотр обновлён · {DateTime.Now:HH:mm:ss}";
        }
        catch (Exception exception) {
            this.StatusText.Foreground = PreviewRenderer.ParseBrush("#FF8A80");
            this.StatusText.Text = exception.Message;
        }
    }

    private void LoadMarkup(string path) {
        this.markupPath = Path.GetFullPath(path);
        this.FilePathText.Text = this.markupPath;
        this.updatingEditors = true;
        this.MarkupEditor.Text = File.ReadAllText(this.markupPath);
        this.updatingEditors = false;
        this.ScheduleRender();
    }

    private void RefreshScenarioNames() {
        var previous = this.ScenarioPicker.SelectedItem as string;
        try {
            using var document = JsonDocument.Parse(this.ScenarioEditor.Text);
            if (document.RootElement.ValueKind != JsonValueKind.Object) {
                return;
            }

            var names = document.RootElement.EnumerateObject().Select(property => property.Name).ToArray();
            this.ScenarioPicker.ItemsSource = names;
            this.ScenarioPicker.SelectedItem = names.Contains(previous) ? previous : names.FirstOrDefault();
        }
        catch (JsonException) {
        }
    }

    private void ScheduleRender() {
        this.renderTimer.Stop();
        this.renderTimer.Start();
    }

    private static string? FindWorkspaceFile(params string[] segments) {
        var directory = new DirectoryInfo(AppContext.BaseDirectory);
        while (directory is not null) {
            var candidate = Path.Combine([directory.FullName, .. segments]);
            if (File.Exists(candidate)) {
                return candidate;
            }

            directory = directory.Parent;
        }

        return null;
    }
}