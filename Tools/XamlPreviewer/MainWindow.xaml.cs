using Microsoft.Win32;
using System.IO;
using System.Text.Json;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;

namespace XamlPreviewer;

public partial class MainWindow : Window {
    private readonly DispatcherTimer renderTimer;
    private readonly DispatcherTimer externalRefreshTimer;
    private readonly MarkupEditorController markupEditorController;
    private FileSystemWatcher? markupWatcher;
    private FileSystemWatcher? scenariosWatcher;
    private FileSystemWatcher? settingsWatcher;
    private PreviewerSettings settings = null!;
    private string? markupPath;
    private bool updatingEditors;
    private EditorMode editorMode;
    private EditorMode previousEditorMode = EditorMode.Xaml;

    private enum EditorMode {
        Xaml,
        Scenarios,
        Settings,
    }

    public MainWindow() {
        InitializeComponent();
        WindowTheme.EnableDarkTitleBar(this);
        this.markupEditorController = new MarkupEditorController(this.MarkupEditor);
        this.markupEditorController.MarkupChanged += this.MarkupEditorControllerMarkupChanged;
        this.renderTimer = new DispatcherTimer {
            Interval = TimeSpan.FromMilliseconds(250)
        };
        this.renderTimer.Tick += this.RenderTimerTick;
        this.externalRefreshTimer = new DispatcherTimer {
            Interval = TimeSpan.FromMilliseconds(200)
        };
        this.externalRefreshTimer.Tick += this.ExternalRefreshTimerTick;
        this.Loaded += this.WindowLoaded;
        this.Closing += this.WindowClosing;
    }

    private void WindowLoaded(object sender, RoutedEventArgs eventArgs) {
        this.LoadSettings();
        this.RestoreWindowState();
        this.ConfigureWatchers();
        this.RefreshPageNames();
        var lastMarkupPath = this.settings.LastMarkupPath;
        if (lastMarkupPath is not null && File.Exists(lastMarkupPath)) {
            this.LoadMarkup(lastMarkupPath);
        } else {
            var defaultMarkupPath = Path.Combine(this.settings.XamlDirectory, "MainPage.xaml");
            if (File.Exists(defaultMarkupPath)) {
                this.LoadMarkup(defaultMarkupPath);
            }
        }

        this.updatingEditors = true;
        this.ScenarioEditor.Text = File.ReadAllText(this.settings.ScenariosPath);
        this.SettingsEditor.Text = this.settings.ToJson();
        this.updatingEditors = false;
        this.RefreshScenarioNames();
        if (this.settings.LastScenarioName is not null) {
            this.ScenarioPicker.SelectedItem = this.settings.LastScenarioName;
        }
        this.UpdateEditorMode();
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
        if (this.editorMode == EditorMode.Scenarios) {
            File.WriteAllText(this.settings.ScenariosPath, this.ScenarioEditor.Text.TrimEnd());
            this.StatusText.Foreground = PreviewRenderer.ParseBrush("#8FD18B");
            this.StatusText.Text = $"Сценарии сохранены: {this.settings.ScenariosPath}";
            return;
        }
        if (this.editorMode == EditorMode.Settings) {
            this.settings = PreviewerSettings.Parse(this.SettingsEditor.Text, this.settings.FilePath);
            this.SaveSettings();
            this.RefreshPageNames();
            this.StatusText.Foreground = PreviewRenderer.ParseBrush("#8FD18B");
            this.StatusText.Text = $"Настройки сохранены: {this.settings.FilePath}";
            return;
        }
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

    private void EditorModeToggleClick(object sender, RoutedEventArgs eventArgs) {
        this.SettingsButton.IsChecked = false;
        this.editorMode = this.EditorModeToggle.IsChecked == true
            ? EditorMode.Scenarios
            : EditorMode.Xaml;
        this.UpdateEditorMode();
    }

    private void SettingsButtonClick(object sender, RoutedEventArgs eventArgs) {
        if (this.SettingsButton.IsChecked == true) {
            this.previousEditorMode = this.editorMode == EditorMode.Settings
                ? this.previousEditorMode
                : this.editorMode;
            this.EditorModeToggle.IsChecked = false;
            this.editorMode = EditorMode.Settings;
        } else {
            this.editorMode = this.previousEditorMode;
            this.EditorModeToggle.IsChecked = this.editorMode == EditorMode.Scenarios;
        }
        this.UpdateEditorMode();
    }

    private void PagePickerSelectionChanged(object sender, SelectionChangedEventArgs eventArgs) {
        if (this.PagePicker.SelectedItem is string pageName) {
            this.LoadMarkup(Path.Combine(this.settings.XamlDirectory, pageName));
        }

        this.RefreshScenarioNames();
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

    private void RenderTimerTick(object? sender, EventArgs eventArgs) {
        this.renderTimer.Stop();
        try {
            using var document = JsonDocument.Parse(this.ScenarioEditor.Text);
            var scenarioName = this.ScenarioPicker.SelectedItem as string;
            var pageName = this.PagePicker.SelectedItem as string;
            var scenarios = pageName is not null
                && document.RootElement.TryGetProperty(Path.GetFileNameWithoutExtension(pageName), out var selectedPage)
                ? selectedPage
                : document.RootElement;
            var data = scenarioName is not null && scenarios.TryGetProperty(scenarioName, out var selected)
                ? selected
                : scenarios;
            this.DeviceSurface.Child = PreviewRenderer.Render(
                this.markupEditorController.Text,
                data,
                this.settings.ResourcesDirectory);
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
        this.ConfigureMarkupWatcher();
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
            if (document.RootElement.ValueKind != JsonValueKind.Object
                || this.PagePicker.SelectedItem is not string pageName
                || !document.RootElement.TryGetProperty(Path.GetFileNameWithoutExtension(pageName), out var scenarios)
                || scenarios.ValueKind != JsonValueKind.Object) {
                return;
            }

            var names = scenarios.EnumerateObject().Select(property => property.Name).ToArray();
            this.ScenarioPicker.ItemsSource = names;
            this.ScenarioPicker.SelectedItem = names.Contains(previous) ? previous : names.FirstOrDefault();
        }
        catch (JsonException) {
        }
    }

    private void RefreshPageNames() {
        var pages = Directory.Exists(this.settings.XamlDirectory)
            ? Directory.GetFiles(this.settings.XamlDirectory, "*.xaml")
                .Select(Path.GetFileName)
                .Where(name => name is not null)
                .Cast<string>()
                .Order()
                .ToArray()
            : [];
        this.PagePicker.ItemsSource = pages;
        this.PagePicker.SelectedItem = pages.Contains("MainPage.xaml")
            ? "MainPage.xaml"
            : pages.FirstOrDefault();
    }

    private void LoadSettings() {
        this.settings = PreviewerSettings.LoadDebug();
    }

    private void ConfigureWatchers() {
        this.ConfigureMarkupWatcher();
        this.scenariosWatcher?.Dispose();
        this.settingsWatcher?.Dispose();
        this.scenariosWatcher = this.CreateWatcher(this.settings.ScenariosPath);
        this.settingsWatcher = this.CreateWatcher(this.settings.FilePath);
    }

    private void ConfigureMarkupWatcher() {
        this.markupWatcher?.Dispose();
        this.markupWatcher = this.markupPath is null ? null : this.CreateWatcher(this.markupPath);
    }

    private FileSystemWatcher? CreateWatcher(string path) {
        var directory = Path.GetDirectoryName(path);
        var fileName = Path.GetFileName(path);
        if (string.IsNullOrEmpty(directory) || string.IsNullOrEmpty(fileName) || !Directory.Exists(directory)) {
            return null;
        }
        var watcher = new FileSystemWatcher(directory, fileName) {
            NotifyFilter = NotifyFilters.LastWrite | NotifyFilters.FileName,
            EnableRaisingEvents = true
        };
        watcher.Changed += this.ExternalFileChanged;
        watcher.Created += this.ExternalFileChanged;
        watcher.Renamed += this.ExternalFileRenamed;
        return watcher;
    }

    private void ExternalFileChanged(object sender, FileSystemEventArgs eventArgs) {
        this.Dispatcher.BeginInvoke(this.QueueExternalRefresh);
    }

    private void ExternalFileRenamed(object sender, RenamedEventArgs eventArgs) {
        this.Dispatcher.BeginInvoke(this.QueueExternalRefresh);
    }

    private void QueueExternalRefresh() {
        this.externalRefreshTimer.Stop();
        this.externalRefreshTimer.Start();
    }

    private void ExternalRefreshTimerTick(object? sender, EventArgs eventArgs) {
        this.externalRefreshTimer.Stop();
        try {
            if (this.markupPath is not null && File.Exists(this.markupPath)) {
                var markup = File.ReadAllText(this.markupPath);
                if (!string.Equals(markup, this.markupEditorController.Text, StringComparison.Ordinal)) {
                    this.updatingEditors = true;
                    this.markupEditorController.SetText(markup);
                    this.updatingEditors = false;
                }
            }
            if (File.Exists(this.settings.ScenariosPath)) {
                var scenarios = File.ReadAllText(this.settings.ScenariosPath);
                if (!string.Equals(scenarios, this.ScenarioEditor.Text, StringComparison.Ordinal)) {
                    this.updatingEditors = true;
                    this.ScenarioEditor.Text = scenarios;
                    this.updatingEditors = false;
                    this.RefreshScenarioNames();
                }
            }
            if (File.Exists(this.settings.FilePath)) {
                var settingsJson = File.ReadAllText(this.settings.FilePath);
                if (!string.Equals(settingsJson, this.SettingsEditor.Text, StringComparison.Ordinal)) {
                    this.settings = PreviewerSettings.Parse(settingsJson, this.settings.FilePath);
                    this.updatingEditors = true;
                    this.SettingsEditor.Text = settingsJson;
                    this.updatingEditors = false;
                    this.ConfigureWatchers();
                    this.RefreshPageNames();
                }
            }
            this.ScheduleRender();
        }
        catch (Exception exception) {
            this.StatusText.Foreground = PreviewRenderer.ParseBrush("#FF8A80");
            this.StatusText.Text = exception.Message;
        }
    }

    private void SaveSettings() {
        this.settings.LastMarkupPath = this.markupPath;
        this.settings.LastScenarioName = this.ScenarioPicker.SelectedItem as string;
        if (this.WindowState == WindowState.Normal) {
            this.settings.WindowWidth = this.Width;
            this.settings.WindowHeight = this.Height;
        }
        this.settings.IsMaximized = this.WindowState == WindowState.Maximized;
        this.settings.EditorPaneWidth = this.EditorColumn.ActualWidth;
        this.settings.Save();
    }

    private void RestoreWindowState() {
        if (this.settings.WindowWidth >= this.MinWidth) {
            this.Width = this.settings.WindowWidth;
        }
        if (this.settings.WindowHeight >= this.MinHeight) {
            this.Height = this.settings.WindowHeight;
        }
        if (this.settings.IsMaximized) {
            this.WindowState = WindowState.Maximized;
        }
        if (this.settings.EditorPaneWidth > 0.0) {
            this.EditorColumn.Width = new GridLength(this.settings.EditorPaneWidth, GridUnitType.Pixel);
        }
    }

    private void WindowClosing(object? sender, System.ComponentModel.CancelEventArgs eventArgs) {
        this.SaveSettings();
        this.markupWatcher?.Dispose();
        this.scenariosWatcher?.Dispose();
        this.settingsWatcher?.Dispose();
    }

    private void UpdateEditorMode() {
        this.MarkupEditor.Visibility = this.editorMode == EditorMode.Xaml ? Visibility.Visible : Visibility.Collapsed;
        this.ScenarioPanel.Visibility = this.editorMode == EditorMode.Scenarios ? Visibility.Visible : Visibility.Collapsed;
        this.SettingsPanel.Visibility = this.editorMode == EditorMode.Settings ? Visibility.Visible : Visibility.Collapsed;
        this.OpenButton.IsEnabled = this.editorMode == EditorMode.Xaml;
        this.XamlModeText.Foreground = PreviewRenderer.ParseBrush(
            this.editorMode == EditorMode.Xaml ? "#D5BD7D" : "#E6E6E6");
        this.ScenariosModeText.Foreground = PreviewRenderer.ParseBrush(
            this.editorMode == EditorMode.Scenarios ? "#D5BD7D" : "#E6E6E6");
    }

    private void ScheduleRender() {
        this.renderTimer.Stop();
        this.renderTimer.Start();
    }
}