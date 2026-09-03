using Microsoft.Win32;
using ICSharpCode.AvalonEdit;
using ICSharpCode.AvalonEdit.Highlighting;
using ICSharpCode.AvalonEdit.Search;
using System.IO;
using System.Reflection;
using System.Text.Json;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;

namespace XamlPreviewer;

/// <summary>
/// Координирует UI превьювера: режимы AvalonEdit, сохранённое состояние сессии,
/// наблюдение за внешними файлами и жизненный цикл нативной сессии рендеринга.
/// Разбор разметки и отрисовка остаются в специализированных классах PreviewRenderer и PreviewSession.
/// </summary>
public partial class MainWindow : Window {
    private readonly DispatcherTimer renderTimer;
    private readonly DispatcherTimer animationTimer;
    private readonly DispatcherTimer externalRefreshTimer;
    private readonly DispatcherTimer smoothScrollTimer;
    private readonly MarkupEditorController markupEditorController;
    private readonly XamlCompletionController xamlCompletionController;
    private readonly Dictionary<TextEditor, SmoothScrollState> smoothScrollStates = [];
    private bool updatingPreviewControls;
    private bool settingsPersistenceReady;
    private FileSystemWatcher? markupWatcher;
    private FileSystemWatcher? scenariosWatcher;
    private FileSystemWatcher? settingsWatcher;
    private PreviewSession? previewSession;
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

    private sealed class DevicePreset {
        public required string Name { get; init; }
        public required int Width { get; init; }
        public required int Height { get; init; }

        public override string ToString() {
            return $"{this.Name} · {this.Width}×{this.Height}";
        }
    }

    private static readonly DevicePreset[] DevicePresets = [
        new() { Name = "Redmi 15C", Width = 720, Height = 1600 },
        new() { Name = "HD+", Width = 720, Height = 1280 },
        new() { Name = "FHD+", Width = 1080, Height = 2400 },
        new() { Name = "QHD+", Width = 1440, Height = 3200 },
    ];

    private sealed class SmoothScrollState {
        public required ScrollViewer ScrollViewer { get; init; }
        public double TargetVerticalOffset { get; set; }
    }

    public MainWindow() {
        InitializeComponent();
        WindowTheme.EnableDarkTitleBar(this);
        MainWindow.ConfigureEditor(this.MarkupEditor, MarkupSyntaxHighlighter.Create());
        MainWindow.ConfigureEditor(this.ScenarioEditor, MarkupSyntaxHighlighter.CreateJson());
        MainWindow.ConfigureEditor(this.SettingsEditor, MarkupSyntaxHighlighter.CreateJson());
        this.markupEditorController = new MarkupEditorController(this.MarkupEditor);
        this.xamlCompletionController = new XamlCompletionController(this.MarkupEditor);
        this.renderTimer = new DispatcherTimer {
            Interval = TimeSpan.FromMilliseconds(250)
        };
        this.renderTimer.Tick += this.RenderTimerTick;
        this.animationTimer = new DispatcherTimer {
            Interval = TimeSpan.FromMilliseconds(16)
        };
        this.animationTimer.Tick += this.AnimationTimerTick;
        this.externalRefreshTimer = new DispatcherTimer {
            Interval = TimeSpan.FromMilliseconds(200)
        };
        this.externalRefreshTimer.Tick += this.ExternalRefreshTimerTick;
        this.smoothScrollTimer = new DispatcherTimer {
            Interval = TimeSpan.FromMilliseconds(16)
        };
        this.smoothScrollTimer.Tick += this.SmoothScrollTimerTick;
        this.Loaded += this.WindowLoaded;
        this.Closing += this.WindowClosing;
        this.SizeChanged += this.WindowSizeChanged;
        this.StateChanged += this.WindowStateChanged;
    }

    private void WindowLoaded(object sender, RoutedEventArgs eventArgs) {
        this.LoadSettings();
        this.ConfigureMouseWheelScrolling();
        this.ApplyEditorScale();
        this.InitializePreviewControls();
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
        this.settingsPersistenceReady = true;
        this.PersistSettings();
        this.ScheduleRender();
        if (this.settings.PreviewScale <= 0.0) {
            this.Dispatcher.BeginInvoke(new Action(this.FitPreview));
        }
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
            this.ConfigureMouseWheelScrolling();
            this.ApplyEditorScale();
            this.ApplySettingsToPreviewControls();
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
        this.PersistSettings();
        this.ScheduleRender();
    }

    private void DevicePresetPickerSelectionChanged(object sender, SelectionChangedEventArgs eventArgs) {
        if (this.updatingPreviewControls || this.DevicePresetPicker.SelectedItem is not DevicePreset preset) {
            return;
        }

        this.settings.PreviewWidth = preset.Width;
        this.settings.PreviewHeight = preset.Height;
        this.ApplyPreviewLayout();
        this.SyncSettingsEditor();
        this.PersistSettings();
        this.ScheduleRender();
    }

    private void PreviewOrientationToggleClick(object sender, RoutedEventArgs eventArgs) {
        if (this.updatingPreviewControls) {
            return;
        }

        this.settings.IsPreviewLandscape = this.PreviewOrientationToggle.IsChecked == true;
        this.UpdatePreviewOrientationToggle();
        this.ApplyPreviewLayout();
        this.SyncSettingsEditor();
        this.PersistSettings();
        this.ScheduleRender();
    }

    private void ZoomOutButtonClick(object sender, RoutedEventArgs eventArgs) {
        this.SetPreviewScale(this.GetPreviewScale() - 0.1);
    }

    private void ZoomInButtonClick(object sender, RoutedEventArgs eventArgs) {
        this.SetPreviewScale(this.GetPreviewScale() + 0.1);
    }

    private void FitPreviewButtonClick(object sender, RoutedEventArgs eventArgs) {
        this.FitPreview();
    }

    private void EditorZoomOutButtonClick(object sender, RoutedEventArgs eventArgs) {
        this.SetEditorScale(this.GetEditorScale() - 0.1);
    }

    private void EditorZoomInButtonClick(object sender, RoutedEventArgs eventArgs) {
        this.SetEditorScale(this.GetEditorScale() + 0.1);
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
            this.SyncSettingsEditor();
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

    private void EditorTextChanged(object sender, EventArgs eventArgs) {
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
        if (this.xamlCompletionController.HandlePreviewKeyDown(eventArgs)) {
            return;
        }
        this.markupEditorController.HandlePreviewKeyDown(eventArgs);
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
            var root = PreviewRenderer.CreateRoot(this.markupEditorController.Text, data);
            var previewSize = this.GetPreviewSize();
            this.animationTimer.Stop();
            this.previewSession?.Dispose();
            this.previewSession = null;
            PreviewSession session;
            try {
                session = new PreviewSession(
                    root,
                    this.settings.ResourcesDirectory,
                    previewSize.Width,
                    previewSize.Height);
            }
            catch {
                NativeRuntime.xr_destroy_element(root);
                throw;
            }
            session.AnimationStarted += this.PreviewSessionAnimationStarted;
            this.previewSession = session;
            this.DeviceSurface.Child = session.Surface;
            this.StatusText.Foreground = PreviewRenderer.ParseBrush("#8FD18B");
            this.StatusText.Text = $"Предпросмотр обновлён · {DateTime.Now:HH:mm:ss}";
        }
        catch (Exception exception) {
            this.StatusText.Foreground = PreviewRenderer.ParseBrush("#FF8A80");
            this.StatusText.Text = exception.Message;
        }
    }

    private void AnimationTimerTick(object? sender, EventArgs eventArgs) {
        if (this.previewSession is null || !this.previewSession.Update()) {
            this.animationTimer.Stop();
        }
    }

    private void PreviewSessionAnimationStarted(object? sender, EventArgs eventArgs) {
        this.animationTimer.Start();
    }

    private void LoadMarkup(string path) {
        this.markupPath = Path.GetFullPath(path);
        this.ConfigureMarkupWatcher();
        this.FilePathText.Text = this.markupPath;
        this.updatingEditors = true;
        this.markupEditorController.SetText(File.ReadAllText(this.markupPath));
        this.updatingEditors = false;
        this.PersistSettings();
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
                    this.ConfigureMouseWheelScrolling();
                    this.ApplyEditorScale();
                    this.ApplySettingsToPreviewControls();
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

    private void PersistSettings() {
        if (this.settingsPersistenceReady) {
            this.SaveSettings();
        }
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
        this.animationTimer.Stop();
        this.smoothScrollTimer.Stop();
        this.previewSession?.Dispose();
        this.markupWatcher?.Dispose();
        this.scenariosWatcher?.Dispose();
        this.settingsWatcher?.Dispose();
    }

    private void WindowSizeChanged(object sender, SizeChangedEventArgs eventArgs) {
        this.PersistSettings();
    }

    private void WindowStateChanged(object? sender, EventArgs eventArgs) {
        this.PersistSettings();
    }

    private void EditorSplitterDragCompleted(object sender, DragCompletedEventArgs eventArgs) {
        this.PersistSettings();
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

    private void InitializePreviewControls() {
        this.DevicePresetPicker.ItemsSource = MainWindow.DevicePresets;
        this.ApplySettingsToPreviewControls();
        this.ApplyPreviewLayout();
    }

    private void ApplySettingsToPreviewControls() {
        this.updatingPreviewControls = true;
        try {
            this.DevicePresetPicker.SelectedItem = MainWindow.DevicePresets.FirstOrDefault(
                preset => preset.Width == this.settings.PreviewWidth
                    && preset.Height == this.settings.PreviewHeight);
            this.PreviewOrientationToggle.IsChecked = this.settings.IsPreviewLandscape;
        }
        finally {
            this.updatingPreviewControls = false;
        }
        this.UpdatePreviewOrientationToggle();
        this.ApplyPreviewLayout();
    }

    private void UpdatePreviewOrientationToggle() {
        this.PortraitOrientationText.Foreground = PreviewRenderer.ParseBrush(
            this.settings.IsPreviewLandscape ? "#E6E6E6" : "#D5BD7D");
        this.LandscapeOrientationText.Foreground = PreviewRenderer.ParseBrush(
            this.settings.IsPreviewLandscape ? "#D5BD7D" : "#E6E6E6");
    }

    private void ApplyPreviewLayout() {
        var previewSize = this.GetPreviewSize();
        var scale = this.GetPreviewScale();
        this.DeviceSurface.Width = previewSize.Width;
        this.DeviceSurface.Height = previewSize.Height;
        this.PreviewViewbox.Width = previewSize.Width * scale;
        this.PreviewViewbox.Height = previewSize.Height * scale;
        this.ZoomText.Text = $"{scale:P0}";
    }

    private void FitPreview() {
        var previewSize = this.GetPreviewSize();
        if (this.PreviewViewport.ViewportWidth <= 0.0 || this.PreviewViewport.ViewportHeight <= 0.0) {
            return;
        }

        var scale = Math.Min(
            this.PreviewViewport.ViewportWidth / previewSize.Width,
            this.PreviewViewport.ViewportHeight / previewSize.Height);
        this.SetPreviewScale(scale);
    }

    private (int Width, int Height) GetPreviewSize() {
        var width = Math.Max(1, this.settings.PreviewWidth);
        var height = Math.Max(1, this.settings.PreviewHeight);
        return this.settings.IsPreviewLandscape
            ? (height, width)
            : (width, height);
    }

    private double GetPreviewScale() {
        return Math.Clamp(
            this.settings.PreviewScale > 0.0 ? this.settings.PreviewScale : 0.5,
            0.1,
            3.0);
    }

    private void SetPreviewScale(double scale) {
        this.settings.PreviewScale = Math.Clamp(scale, 0.1, 3.0);
        this.ApplyPreviewLayout();
        this.SyncSettingsEditor();
        this.PersistSettings();
    }

    private void ApplyEditorScale() {
        const double defaultFontSize = 14.0;
        var scale = this.GetEditorScale();
        foreach (var editor in new[] {
            this.MarkupEditor,
            this.ScenarioEditor,
            this.SettingsEditor,
        }) {
            editor.FontSize = defaultFontSize * scale;
        }
        this.EditorZoomText.Text = $"{scale:P0}";
    }

    private double GetEditorScale() {
        return Math.Clamp(
            this.settings.EditorScale > 0.0 ? this.settings.EditorScale : 1.0,
            0.5,
            3.0);
    }

    private void SetEditorScale(double scale) {
        this.settings.EditorScale = Math.Clamp(scale, 0.5, 3.0);
        this.ApplyEditorScale();
        this.SyncSettingsEditor();
        this.PersistSettings();
    }

    private void SyncSettingsEditor() {
        if (this.editorMode != EditorMode.Settings) {
            return;
        }

        this.updatingEditors = true;
        this.SettingsEditor.Text = this.settings.ToJson();
        this.updatingEditors = false;
    }

    private static void ConfigureEditor(TextEditor editor, IHighlightingDefinition highlighting) {
        editor.SyntaxHighlighting = highlighting;
        editor.TextArea.Caret.CaretBrush = PreviewRenderer.ParseBrush("#F0D78C");
        editor.TextArea.SelectionBrush = PreviewRenderer.ParseBrush("#5A4D26");
        editor.TextArea.SelectionForeground = PreviewRenderer.ParseBrush("#FFFFFF");
        var searchPanel = SearchPanel.Install(editor);
        searchPanel.Background = PreviewRenderer.ParseBrush("#252525");
        searchPanel.BorderBrush = PreviewRenderer.ParseBrush("#4A4A4A");
        searchPanel.Foreground = PreviewRenderer.ParseBrush("#E6E6E6");
        searchPanel.MarkerBrush = PreviewRenderer.ParseBrush("#665A4D26");
        searchPanel.MarkerPen = new Pen(PreviewRenderer.ParseBrush("#D5BD7D"), 1.0);
        var messageField = typeof(SearchPanel).GetField("messageView", BindingFlags.Instance | BindingFlags.NonPublic);
        if (messageField?.GetValue(searchPanel) is ToolTip message) {
            message.Visibility = Visibility.Collapsed;
        }
    }

    private void ConfigureMouseWheelScrolling() {
        this.smoothScrollTimer.Stop();
        this.smoothScrollStates.Clear();
        foreach (var editor in new[] {
            this.MarkupEditor,
            this.ScenarioEditor,
            this.SettingsEditor,
        }) {
            editor.PreviewMouseWheel -= this.EditorPreviewMouseWheel;
            if (this.settings.MouseWheelLines > 0) {
                editor.PreviewMouseWheel += this.EditorPreviewMouseWheel;
            }
        }
    }

    private void EditorPreviewMouseWheel(object sender, MouseWheelEventArgs eventArgs) {
        if (sender is not TextEditor editor || this.settings.MouseWheelLines <= 0) {
            return;
        }

        var scrollViewer = MainWindow.FindVisualChild<ScrollViewer>(editor);
        if (scrollViewer is null) {
            return;
        }

        var state = this.smoothScrollStates.GetValueOrDefault(editor);
        if (state is null || !ReferenceEquals(state.ScrollViewer, scrollViewer)) {
            state = new SmoothScrollState {
                ScrollViewer = scrollViewer,
                TargetVerticalOffset = scrollViewer.VerticalOffset,
            };
            this.smoothScrollStates[editor] = state;
        }

        var steps = Math.Max(1, Math.Abs(eventArgs.Delta) / Mouse.MouseWheelDeltaForOneLine);
        var lineHeight = editor.TextArea.TextView.DefaultLineHeight;
        var offset = steps * this.settings.MouseWheelLines * lineHeight;
        state.TargetVerticalOffset = Math.Clamp(
            state.TargetVerticalOffset + (eventArgs.Delta > 0 ? -offset : offset),
            0.0,
            scrollViewer.ScrollableHeight);
        this.smoothScrollTimer.Start();
        eventArgs.Handled = true;
    }

    private void SmoothScrollTimerTick(object? sender, EventArgs eventArgs) {
        var isAnimating = false;
        foreach (var state in this.smoothScrollStates.Values) {
            var currentOffset = state.ScrollViewer.VerticalOffset;
            var difference = state.TargetVerticalOffset - currentOffset;
            if (Math.Abs(difference) <= 0.5) {
                state.ScrollViewer.ScrollToVerticalOffset(state.TargetVerticalOffset);
                continue;
            }

            state.ScrollViewer.ScrollToVerticalOffset(currentOffset + difference * 0.35);
            isAnimating = true;
        }
        if (!isAnimating) {
            this.smoothScrollTimer.Stop();
        }
    }

    private static T? FindVisualChild<T>(DependencyObject element)
        where T : DependencyObject {
        for (var index = 0; index < VisualTreeHelper.GetChildrenCount(element); ++index) {
            var child = VisualTreeHelper.GetChild(element, index);
            if (child is T result) {
                return result;
            }

            var recursiveResult = MainWindow.FindVisualChild<T>(child);
            if (recursiveResult is not null) {
                return recursiveResult;
            }
        }
        return null;
    }

    private void ScheduleRender() {
        this.renderTimer.Stop();
        this.renderTimer.Start();
    }
}