using Microsoft.Win32;
using System.IO;
using System.Text.Json;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;

namespace XamlPreviewer;

public partial class MainWindow : Window {
    private const string MarkupPath = @"C:\WORK\TEST\XamlPreviewer\MainPage.xaml";
    private const string ScenariosPath = @"C:\WORK\TEST\XamlPreviewer\scenarios.json";

    private readonly DispatcherTimer renderTimer;
    private readonly MarkupEditorController markupEditorController;
    private string? markupPath;
    private bool updatingEditors;

    public MainWindow() {
        InitializeComponent();
        WindowTheme.EnableDarkTitleBar(this);
        this.markupEditorController = new MarkupEditorController(this.MarkupEditor);
        this.markupEditorController.MarkupChanged += this.MarkupEditorControllerMarkupChanged;
        this.SyntaxHighlightingCheckBox.IsChecked = true;
        this.renderTimer = new DispatcherTimer {
            Interval = TimeSpan.FromMilliseconds(250)
        };
        this.renderTimer.Tick += this.RenderTimerTick;
        this.Loaded += this.WindowLoaded;
    }

    private void WindowLoaded(object sender, RoutedEventArgs eventArgs) {
        if (File.Exists(MarkupPath)) {
            this.LoadMarkup(MarkupPath);
        }

        this.updatingEditors = true;
        this.ScenarioEditor.Text = File.Exists(ScenariosPath)
            ? File.ReadAllText(ScenariosPath)
            : "{\n  \"Default\": {}\n}";
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

        File.WriteAllText(this.markupPath, this.markupEditorController.Text.TrimEnd());
        this.StatusText.Foreground = PreviewRenderer.ParseBrush("#8FD18B");
        this.StatusText.Text = $"Сохранено: {this.markupPath}";
    }

    private void ScenarioPickerSelectionChanged(object sender, SelectionChangedEventArgs eventArgs) {
        this.ScheduleRender();
    }

    private void EditorTextChanged(object sender, TextChangedEventArgs eventArgs) {
        if (ReferenceEquals(sender, this.MarkupEditor)) {
            if (this.markupEditorController.HandleTextChanged()) {
                this.ScheduleRender();
            }

            return;
        }

        if (!this.updatingEditors) {
            if (ReferenceEquals(sender, this.ScenarioEditor)) {
                this.RefreshScenarioNames();
            }

            this.ScheduleRender();
        }
    }

    private void MarkupEditorPreviewKeyDown(object sender, KeyEventArgs eventArgs) {
        this.markupEditorController.HandlePreviewKeyDown(eventArgs);
    }

    private void MarkupEditorControllerMarkupChanged(object? sender, EventArgs eventArgs) {
        this.ScheduleRender();
    }

    private void SyntaxHighlightingCheckBoxChanged(object sender, RoutedEventArgs eventArgs) {
        this.markupEditorController.SetSyntaxHighlightingEnabled(this.SyntaxHighlightingCheckBox.IsChecked == true);
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
            this.DeviceSurface.Child = PreviewRenderer.Render(
                this.markupEditorController.Text,
                data,
                this.markupPath);
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
        this.markupEditorController.SetText(File.ReadAllText(this.markupPath));
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
}