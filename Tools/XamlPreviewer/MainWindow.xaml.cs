using ICSharpCode.AvalonEdit;
using ICSharpCode.AvalonEdit.Highlighting;
using ICSharpCode.AvalonEdit.Search;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;

namespace XamlPreviewer;

#pragma warning disable CS0162

/// <summary>
/// Координирует UI превьювера: режимы AvalonEdit, сохранённое состояние сессии,
/// наблюдение за внешними файлами и жизненный цикл нативной сессии рендеринга.
/// Разбор разметки и отрисовка остаются в специализированных классах PreviewRenderer и PreviewSession.
/// </summary>
public partial class MainWindow : Window {
    private const string NativeBridgeLibraryName = "XamlRuntime.NativeBridge.dll";
    private readonly DispatcherTimer renderTimer;
    private readonly DispatcherTimer animationTimer;
    private readonly DispatcherTimer previewZoomTimer;
    private readonly DispatcherTimer externalRefreshTimer;
    private readonly DispatcherTimer smoothScrollTimer;
    private readonly MarkupEditorController markupEditorController;
    private readonly XamlCompletionController xamlCompletionController;
    private readonly FolderPickerController folderPickerController;
    private readonly PreviewViewportController previewViewportController;
    private readonly Grid previewLayer = new();
    private readonly Dictionary<TextEditor, SmoothScrollState> smoothScrollStates = [];
    private bool updatingPreviewControls;
    private bool settingsPersistenceReady;
    private FileSystemWatcher? markupWatcher;
    private FileSystemWatcher? xamlDirectoryWatcher;
    private FileSystemWatcher? scenariosWatcher;
    private FileSystemWatcher? settingsWatcher;
    private PreviewSession? previewSession;
    private bool isClosing;
    private PreviewSession? outgoingSession;
    private PreviewerSettings settings = null!;
    private string? markupPath;
    private (string From, string To, bool Backward)? pendingPageTransition;
    private (int Width, int Height)? markupPreviewResolution;
    private bool shouldRestorePreviewPosition = true;
    private bool isMarkupDirty;
    private bool isScenariosDirty;
    private bool isSettingsDirty;
    private bool updatingEditors;
    private EditorMode editorMode;
    private EditorMode previousEditorMode = EditorMode.Xaml;
    private string? folderPickerDirectory;
    private readonly List<FolderPickerHistoryEntry> folderPickerHistory = [];
    private int folderPickerHistoryIndex = -1;
    private bool isPreviewPanning;
    private Point previewPanStart;
    private double previewPanHorizontalOffset;
    private double previewPanVerticalOffset;
    private DateTime previewZoomStartedAt;
    private double previewZoomStartScale;
    private double previewZoomTargetScale;
    private double previewZoomAnchorX;
    private double previewZoomAnchorY;
    private Point previewZoomViewportPoint;

    private sealed class FolderPickerEntry {
        public required string FullPath { get; init; }
        public required string Name { get; init; }
        public required bool IsDirectory { get; init; }

        public override string ToString() {
            return this.IsDirectory ? $"📁  {this.Name}" : $"     {this.Name}";
        }
    }

    private sealed class FolderPickerHistoryEntry {
        public required string DirectoryPath { get; init; }
        public string? SelectedEntryPath { get; set; }
    }

    private enum EditorMode {
        Xaml,
        Scenarios,
        Settings,
    }

    private sealed class AnimationSpeed {
        public required string Name { get; init; }
        public required double Rate { get; init; }

        public override string ToString() {
            return this.Name;
        }
    }

    private enum ScrollSmoothingMode {
        None,
        Linear,
        Smoothstep,
        Smootherstep,
        EaseOutCubic,
        Exponential,
    }

    private sealed class SmoothScrollState {
        public required ScrollViewer ScrollViewer { get; init; }
        public double StartVerticalOffset { get; set; }
        public double TargetVerticalOffset { get; set; }
        public double LastAppliedVerticalOffset { get; set; }
        public long AnimationStartedAt { get; set; }
        public TimeSpan AnimationDuration { get; set; }
        public ScrollSmoothingMode SmoothingMode { get; set; }
        public bool IsAnimating { get; set; }
    }

    public MainWindow() {
        InitializeComponent();
        this.DeviceSurface.Child = this.previewLayer;
        WindowTheme.EnableDarkTitleBar(this);
        MainWindow.ConfigureEditor(this.MarkupEditor, MarkupSyntaxHighlighter.Create());
        MainWindow.ConfigureEditor(this.ScenarioEditor, MarkupSyntaxHighlighter.CreateJson());
        MainWindow.ConfigureEditor(this.SettingsEditor, MarkupSyntaxHighlighter.CreateJson());
        this.markupEditorController = new MarkupEditorController(this.MarkupEditor);
        this.xamlCompletionController = new XamlCompletionController(this.MarkupEditor);
        this.folderPickerController = new FolderPickerController(
            this.FolderPickerPanel,
            this.FolderPickerPathText,
            this.FolderPickerErrorText,
            this.FolderPickerEntries,
            this.SelectFolderButton,
            this.FolderPickerBackButton,
            this.FolderPickerForwardButton,
            this.ShowFolderPickerPreview,
            this.ClearFolderPickerPreview,
            this.ReportFolderPickerStatus);
        this.previewViewportController = new PreviewViewportController(
            this.PreviewViewport,
            this.GetPreviewScale,
            this.SetPreviewScale,
            () => { this.SyncSettingsEditor(); this.PersistSettings(); });
        this.renderTimer = new DispatcherTimer {
            Interval = TimeSpan.FromMilliseconds(250)
        };
        this.renderTimer.Tick += this.RenderTimerTick;
        this.animationTimer = new DispatcherTimer {
            Interval = TimeSpan.FromMilliseconds(16)
        };
        this.animationTimer.Tick += this.AnimationTimerTick;
        this.previewZoomTimer = new DispatcherTimer {
            Interval = TimeSpan.FromMilliseconds(16)
        };
        this.previewZoomTimer.Tick += this.PreviewZoomTimerTick;
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
        this.PreviewKeyDown += this.WindowPreviewKeyDown;
        this.PreviewKeyUp += this.WindowPreviewKeyUp;
        this.Deactivated += (_, _) => this.previewSession?.SetElementInspectionEnabled(false);
        this.Activated += (_, _) => this.UpdateElementInspection();
    }

    private void WindowLoaded(object sender, RoutedEventArgs eventArgs) {
        this.UpdateNativeBridgeTitle();
        NativeRuntime.xr_configure_logging(Path.Combine(AppContext.BaseDirectory, "xaml-previewer.log"));
        this.LoadSettings();
        this.ConfigureMouseWheelScrolling();
        this.ApplyEditorScale();
        this.InitializePreviewControls();
        this.RestoreWindowState();
        this.ConfigureWatchers();
        this.RefreshPageNames();
        var lastMarkupPath = this.settings.LastMarkupPath;
        if (lastMarkupPath is not null && File.Exists(lastMarkupPath)) {
            var lastPageName = Path.GetRelativePath(this.settings.XamlDirectory, lastMarkupPath);
            if (this.PagePicker.Items.Contains(lastPageName)) {
                this.PagePicker.SelectedItem = lastPageName;
            } else {
                this.LoadMarkup(lastMarkupPath);
            }
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
        this.isMarkupDirty = false;
        this.isScenariosDirty = false;
        this.isSettingsDirty = false;
        this.RefreshScenarioNames();
        if (this.settings.LastScenarioName is not null) {
            this.ScenarioPicker.SelectedItem = this.settings.LastScenarioName;
        }
        this.UpdateEditorMode();
        this.settingsPersistenceReady = true;
        this.PersistSettings();
        this.UpdateDocumentState();
        this.ScheduleRender();
        if (this.settings.PreviewScale <= 0.0) {
            this.Dispatcher.BeginInvoke(new Action(this.FitPreview));
        }
    }

    private void UpdateNativeBridgeTitle() {
        var loadedLibraryPath = Path.Combine(AppContext.BaseDirectory, NativeBridgeLibraryName);
        var loadedLibrary = new FileInfo(loadedLibraryPath);
        if (!loadedLibrary.Exists) {
            this.Title = "MobileClock XAML Previewer (DLL не найдена)";
            WindowTheme.SetTitleBarWarning(this, true);
            return;
        }

        var expectedLibraryPath = MainWindow.GetExpectedNativeBridgePath();
        var isCurrent = expectedLibraryPath is not null
            && File.Exists(expectedLibraryPath)
            && MainWindow.FilesAreEqual(loadedLibraryPath, expectedLibraryPath);
        this.Title = $"MobileClock XAML Previewer (DLL: {loadedLibrary.LastWriteTime:yyyy-MM-dd HH:mm:ss}{(isCurrent ? string.Empty : " — СТАРАЯ")})";
        WindowTheme.SetTitleBarWarning(this, !isCurrent);
    }

    private static string? GetExpectedNativeBridgePath() {
        var outputDirectory = new DirectoryInfo(AppContext.BaseDirectory);
        var configurationDirectory = outputDirectory.Parent?.Parent;
        if (configurationDirectory?.Parent?.Name != "Build") {
            return null;
        }

        return Path.Combine(
            configurationDirectory.FullName,
            "x64",
            "XamlRuntime.NativeBridge",
            NativeBridgeLibraryName);
    }

    private static bool FilesAreEqual(string firstPath, string secondPath) {
        var first = new FileInfo(firstPath);
        var second = new FileInfo(secondPath);
        return first.Length == second.Length
            && CryptographicOperations.FixedTimeEquals(
                SHA256.HashData(File.ReadAllBytes(firstPath)),
                SHA256.HashData(File.ReadAllBytes(secondPath)));
    }

    private void OpenButtonClick(object sender, RoutedEventArgs eventArgs) {
        this.folderPickerController.Open(this.settings.XamlDirectory);
        this.MarkupEditor.Visibility = Visibility.Collapsed;
        this.ScenarioPanel.Visibility = Visibility.Collapsed;
        this.SettingsPanel.Visibility = Visibility.Collapsed;
        this.OpenButton.IsEnabled = false;
        this.StatusText.Foreground = PreviewRenderer.ParseBrush("#D5BD7D");
        this.StatusText.Text = "Выберите папку, содержащую XAML-файлы.";
    }

    private void FolderPickerUpButtonClick(object sender, RoutedEventArgs eventArgs) {
        this.folderPickerController.Up();
    }

    private void FolderPickerBackButtonClick(object sender, RoutedEventArgs eventArgs) {
        this.folderPickerController.Back();
    }

    private void FolderPickerForwardButtonClick(object sender, RoutedEventArgs eventArgs) {
        this.folderPickerController.Forward();
    }

    private void FolderPickerPathTextKeyDown(object sender, KeyEventArgs eventArgs) {
        if (eventArgs.Key != Key.Enter) {
            return;
        }
        this.folderPickerController.SubmitPath();
        eventArgs.Handled = true;
    }

    private void FolderPickerEntriesPreviewKeyDown(object sender, KeyEventArgs eventArgs) {
        this.folderPickerController.HandleListKey(eventArgs);
    }

    private void FolderPickerEntriesPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs eventArgs) {
        this.folderPickerController.HandleListBackgroundMouseDown(eventArgs.OriginalSource as DependencyObject);
    }

    private void FolderPickerEntriesMouseDoubleClick(object sender, MouseButtonEventArgs eventArgs) {
        this.folderPickerController.HandleListDoubleClick();
    }

    private void FolderPickerEntriesSelectionChanged(object sender, SelectionChangedEventArgs eventArgs) {
        this.folderPickerController.HandleSelectionChanged();
    }

    private void CancelFolderPickerButtonClick(object sender, RoutedEventArgs eventArgs) {
        this.HideFolderPicker();
    }

    private void SelectFolderButtonClick(object sender, RoutedEventArgs eventArgs) {
        if (!this.folderPickerController.TrySelectDirectory(out var selectedDirectory)) {
            return;
        }
        this.settings.XamlDirectory = selectedDirectory;
        this.ConfigureWatchers();
        this.RefreshPageNames();
        this.HideFolderPicker();
        this.PersistSettings();
        this.StatusText.Foreground = PreviewRenderer.ParseBrush("#8FD18B");
        this.StatusText.Text = $"Выбрана папка XAML: {this.settings.XamlDirectory}";
    }

    private void RefreshFolderPickerEntries(string? selectedEntryPath) {
        if (this.folderPickerDirectory is null || !Directory.Exists(this.folderPickerDirectory)) {
            return;
        }
        this.FolderPickerPathText.Text = this.folderPickerDirectory;
        this.FolderPickerErrorText.Text = string.Empty;
        var entries = Directory.EnumerateDirectories(this.folderPickerDirectory)
            .Order()
            .Select(path => new FolderPickerEntry {
                FullPath = path,
                Name = Path.GetFileName(path),
                IsDirectory = true,
            })
            .Concat(Directory.EnumerateFiles(this.folderPickerDirectory, "*.xaml")
                .Order()
                .Select(path => new FolderPickerEntry {
                    FullPath = path,
                    Name = Path.GetFileName(path),
                    IsDirectory = false,
                }))
            .ToArray();
        this.FolderPickerEntries.ItemsSource = entries;
        this.FolderPickerEntries.UnselectAll();
        if (selectedEntryPath is not null) {
            var selectedEntry = entries.FirstOrDefault(entry => string.Equals(
                entry.FullPath,
                selectedEntryPath,
                StringComparison.OrdinalIgnoreCase));
            this.FolderPickerEntries.SelectedItem = selectedEntry;
            if (selectedEntry is not null) {
                this.FocusFolderPickerEntry(selectedEntry);
            }
        }
        this.UpdateFolderPickerSelection();
    }

    private void FocusFolderPickerEntry(FolderPickerEntry entry) {
        this.Dispatcher.BeginInvoke(DispatcherPriority.Input, new Action(() => {
            this.FolderPickerEntries.ScrollIntoView(entry);
            if (this.FolderPickerEntries.ItemContainerGenerator.ContainerFromItem(entry) is ListBoxItem item) {
                Keyboard.Focus(item);
                return;
            }
            this.FolderPickerEntries.Focus();
        }));
    }

    private bool NavigateFolderPicker(string candidate, bool addToHistory, string? selectedEntryPath = null) {
        string path;
        try {
            path = Path.GetFullPath(candidate);
        }
        catch (Exception exception) when (exception is ArgumentException or NotSupportedException or PathTooLongException) {
            this.ShowFolderPickerError("Указан некорректный путь.");
            return false;
        }
        if (!Directory.Exists(path)) {
            this.ShowFolderPickerError("Папка не существует или недоступна.");
            return false;
        }
        if (addToHistory) {
            this.SaveFolderPickerSelection();
        }
        this.folderPickerDirectory = path;
        if (addToHistory) {
            if (this.folderPickerHistoryIndex < this.folderPickerHistory.Count - 1) {
                this.folderPickerHistory.RemoveRange(
                    this.folderPickerHistoryIndex + 1,
                    this.folderPickerHistory.Count - this.folderPickerHistoryIndex - 1);
            }
            this.folderPickerHistory.Add(new FolderPickerHistoryEntry {
                DirectoryPath = path
            });
            this.folderPickerHistoryIndex = this.folderPickerHistory.Count - 1;
        }
        if (selectedEntryPath is null
            && this.folderPickerHistoryIndex >= 0
            && this.folderPickerHistoryIndex < this.folderPickerHistory.Count) {
            selectedEntryPath = this.folderPickerHistory[this.folderPickerHistoryIndex].SelectedEntryPath;
        }
        this.RefreshFolderPickerEntries(selectedEntryPath);
        this.FolderPickerBackButton.IsEnabled = this.folderPickerHistoryIndex > 0;
        this.FolderPickerForwardButton.IsEnabled = this.folderPickerHistoryIndex < this.folderPickerHistory.Count - 1;
        return true;
    }

    private void SaveFolderPickerSelection() {
        if (this.folderPickerHistoryIndex < 0
            || this.folderPickerHistoryIndex >= this.folderPickerHistory.Count) {
            return;
        }
        this.folderPickerHistory[this.folderPickerHistoryIndex].SelectedEntryPath =
            (this.FolderPickerEntries.SelectedItem as FolderPickerEntry)?.FullPath;
    }

    private void ShowFolderPickerError(string message) {
        this.FolderPickerErrorText.Text = message;
        this.StatusText.Foreground = PreviewRenderer.ParseBrush("#FF8A80");
        this.StatusText.Text = message;
    }

    private void UpdateFolderPickerSelection() {
        this.SelectFolderButton.IsEnabled = this.FolderPickerEntries.SelectedItem is not FolderPickerEntry entry
            || entry.IsDirectory;
    }

    private void ShowFolderPickerPreview(string path) {
        try {
            using var document = JsonDocument.Parse(this.ScenarioEditor.Text);
            var pageName = Path.GetFileName(path);
            var scenarios = document.RootElement.TryGetProperty(
                Path.GetFileNameWithoutExtension(pageName),
                out var selectedPage)
                ? selectedPage
                : document.RootElement;
            var scenarioName = this.ScenarioPicker.SelectedItem as string;
            var data = scenarioName is not null && scenarios.TryGetProperty(scenarioName, out var selected)
                ? selected
                : scenarios;
            var root = PreviewRenderer.CreateRoot(File.ReadAllText(path), data);
            this.animationTimer.Stop();
            this.CompletePageTransition();
            this.previewSession?.Dispose();
            this.previewSession = null;
            this.previewLayer.Children.Clear();
            var previewSize = this.GetPreviewSize();
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
            session.SetAnimationSpeed(this.GetAnimationPlaybackRate());
            session.AnimationStarted += this.PreviewSessionAnimationStarted;
            this.animationTimer.Start();
            session.Tapped += this.PreviewSessionTapped;
            this.previewSession = session;
            this.previewLayer.Children.Add(session.Surface);
            this.StatusText.Foreground = PreviewRenderer.ParseBrush("#8FD18B");
            this.StatusText.Text = $"Предпросмотр: {path}";
        }
        catch (Exception exception) {
            this.ShowPreviewError(exception);
        }
    }

    private void ClearFolderPickerPreview() {
        this.animationTimer.Stop();
        this.pendingPageTransition = null;
        this.CompletePageTransition();
        this.previewSession?.Dispose();
        this.previewSession = null;
        this.previewLayer.Children.Clear();
    }

    private void ShowPreviewError(Exception exception) {
        this.ClearFolderPickerPreview();
        this.previewLayer.Children.Add(new Border {
            Background = PreviewRenderer.ParseBrush("#1F1717"),
            BorderBrush = PreviewRenderer.ParseBrush("#A75B5B"),
            BorderThickness = new Thickness(1),
            Child = new TextBlock {
                Margin = new Thickness(32),
                Foreground = PreviewRenderer.ParseBrush("#FFB4AB"),
                FontSize = 18,
                Text = $"Ошибка предпросмотра\n\n{exception.Message}",
                TextWrapping = TextWrapping.Wrap,
                VerticalAlignment = VerticalAlignment.Center,
                HorizontalAlignment = HorizontalAlignment.Center,
                TextAlignment = TextAlignment.Center,
            },
        });
        this.StatusText.Foreground = PreviewRenderer.ParseBrush("#FF8A80");
        this.StatusText.Text = exception.Message;
    }

    private void HideFolderPicker() {
        this.folderPickerController.Close();
        this.folderPickerDirectory = null;
        this.folderPickerHistory.Clear();
        this.folderPickerHistoryIndex = -1;
        this.UpdateEditorMode();
        this.ScheduleRender();
    }

    private void ReportFolderPickerStatus(string message, bool isSuccess) {
        this.StatusText.Foreground = PreviewRenderer.ParseBrush(isSuccess ? "#8FD18B" : "#D5BD7D");
        this.StatusText.Text = message;
    }

    private void SaveButtonClick(object sender, RoutedEventArgs eventArgs) {
        if (this.editorMode == EditorMode.Scenarios) {
            File.WriteAllText(this.settings.ScenariosPath, this.ScenarioEditor.Text.TrimEnd());
            this.isScenariosDirty = false;
            this.UpdateDocumentState();
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
            this.updatingEditors = true;
            this.SettingsEditor.Text = this.settings.ToJson();
            this.updatingEditors = false;
            this.isSettingsDirty = false;
            this.UpdateDocumentState();
            this.StatusText.Foreground = PreviewRenderer.ParseBrush("#8FD18B");
            this.StatusText.Text = $"Настройки сохранены: {this.settings.FilePath}";
            return;
        }
        if (this.markupPath is null) {
            return;
        }

        File.WriteAllText(this.markupPath, this.markupEditorController.Text.TrimEnd());
        this.isMarkupDirty = false;
        this.UpdateDocumentState();
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

    private void AnimationSpeedPickerSelectionChanged(object sender, SelectionChangedEventArgs eventArgs) {
        if (this.updatingPreviewControls || this.AnimationSpeedPicker.SelectedItem is not AnimationSpeed speed) {
            return;
        }

        this.settings.AnimationPlaybackRate = speed.Rate;
        this.previewSession?.SetAnimationSpeed(speed.Rate);
        this.outgoingSession?.SetAnimationSpeed(speed.Rate);
        this.SyncSettingsEditor();
        this.PersistSettings();
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

    private void PreviewViewportPreviewMouseWheel(object sender, MouseWheelEventArgs eventArgs) {
        this.previewViewportController.HandleMouseWheel(eventArgs);
        return;
        if (!Keyboard.IsKeyDown(Key.LeftCtrl) && !Keyboard.IsKeyDown(Key.RightCtrl)) {
            return;
        }
        var point = eventArgs.GetPosition(this.PreviewViewport);
        this.previewZoomStartScale = this.GetPreviewScale();
        var wheelSteps = Math.Max(1, Math.Abs(eventArgs.Delta) / Mouse.MouseWheelDeltaForOneLine);
        var zoomFactor = Math.Pow(1.25, wheelSteps);
        this.previewZoomTargetScale = Math.Clamp(
            this.previewZoomStartScale * (eventArgs.Delta > 0 ? zoomFactor : 1.0 / zoomFactor),
            0.1,
            3.0);
        this.previewZoomAnchorX = (this.PreviewViewport.HorizontalOffset + point.X) / this.previewZoomStartScale;
        this.previewZoomAnchorY = (this.PreviewViewport.VerticalOffset + point.Y) / this.previewZoomStartScale;
        this.previewZoomViewportPoint = point;
        this.previewZoomStartedAt = DateTime.UtcNow;
        this.previewZoomTimer.Start();
        eventArgs.Handled = true;
    }

    private void PreviewZoomTimerTick(object? sender, EventArgs eventArgs) {
        const double durationMilliseconds = 120.0;
        var progress = Math.Clamp((DateTime.UtcNow - this.previewZoomStartedAt).TotalMilliseconds / durationMilliseconds, 0.0, 1.0);
        var easedProgress = progress;
        // var inverseProgress = 1.0 - progress;
        // var easedProgress = 1.0 - inverseProgress * inverseProgress * inverseProgress;
        var scale = this.previewZoomStartScale
            + (this.previewZoomTargetScale - this.previewZoomStartScale) * easedProgress;
        this.settings.PreviewScale = scale;
        this.ApplyPreviewLayout();
        this.PreviewViewport.ScrollToHorizontalOffset(
            this.previewZoomAnchorX * scale - this.previewZoomViewportPoint.X);
        this.PreviewViewport.ScrollToVerticalOffset(
            this.previewZoomAnchorY * scale - this.previewZoomViewportPoint.Y);
        if (progress < 1.0) {
            return;
        }
        this.previewZoomTimer.Stop();
        this.SyncSettingsEditor();
        this.PersistSettings();
    }

    private void PreviewViewportPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs eventArgs) {
        this.previewViewportController.HandleMouseDown(eventArgs);
        return;
        if ((Keyboard.Modifiers & ModifierKeys.Control) == 0) {
            return;
        }
        this.isPreviewPanning = true;
        this.previewPanStart = eventArgs.GetPosition(this.PreviewViewport);
        this.previewPanHorizontalOffset = this.PreviewViewport.HorizontalOffset;
        this.previewPanVerticalOffset = this.PreviewViewport.VerticalOffset;
        this.PreviewViewport.Cursor = Cursors.Cross;
        this.PreviewViewport.CaptureMouse();
        eventArgs.Handled = true;
    }

    private void PreviewViewportPreviewMouseMove(object sender, MouseEventArgs eventArgs) {
        this.previewViewportController.HandleMouseMove(eventArgs);
        return;
        if (!this.isPreviewPanning) {
            return;
        }
        var point = eventArgs.GetPosition(this.PreviewViewport);
        this.PreviewViewport.ScrollToHorizontalOffset(this.previewPanHorizontalOffset - point.X + this.previewPanStart.X);
        this.PreviewViewport.ScrollToVerticalOffset(this.previewPanVerticalOffset - point.Y + this.previewPanStart.Y);
        eventArgs.Handled = true;
    }

    private void PreviewViewportPreviewMouseLeftButtonUp(object sender, MouseButtonEventArgs eventArgs) {
        this.previewViewportController.HandleMouseUp(eventArgs);
        return;
        if (!this.isPreviewPanning) {
            return;
        }
        this.StopPreviewPanning();
        eventArgs.Handled = true;
    }

    private void PreviewViewportLostMouseCapture(object sender, MouseEventArgs eventArgs) {
        this.previewViewportController.HandleLostMouseCapture();
        return;
        this.StopPreviewPanning();
    }

    private void WindowPreviewKeyDown(object sender, KeyEventArgs eventArgs) {
        var key = eventArgs.Key == Key.System ? eventArgs.SystemKey : eventArgs.Key;
        if (key is Key.LeftAlt or Key.RightAlt) {
            this.UpdateElementInspection();
        }
    }

    private void WindowPreviewKeyUp(object sender, KeyEventArgs eventArgs) {
        var key = eventArgs.Key == Key.System ? eventArgs.SystemKey : eventArgs.Key;
        if (key is Key.LeftAlt or Key.RightAlt) {
            this.UpdateElementInspection();
        }
        this.previewViewportController.HandleKeyUp(eventArgs);
        return;
        if (eventArgs.Key == Key.LeftCtrl || eventArgs.Key == Key.RightCtrl) {
            this.StopPreviewPanning();
        }
    }

    private void StopPreviewPanning() {
        if (!this.isPreviewPanning) {
            return;
        }
        this.isPreviewPanning = false;
        this.PreviewViewport.Cursor = null;
        this.PreviewViewport.ReleaseMouseCapture();
    }

    private void UpdateElementInspection() {
        if (this.isClosing) {
            return;
        }
        this.previewSession?.SetElementInspectionEnabled(
            this.IsActive && this.editorMode == EditorMode.Xaml
            && (Keyboard.IsKeyDown(Key.LeftAlt) || Keyboard.IsKeyDown(Key.RightAlt)));
    }

    private void PreviewElementSelected(object? sender, (int Line, int Column) location) {
        if (this.editorMode != EditorMode.Xaml || !ReferenceEquals(sender, this.previewSession)) {
            return;
        }
        var document = this.MarkupEditor.Document;
        if (location.Line < 1 || location.Line > document.LineCount) {
            return;
        }
        var line = document.GetLineByNumber(location.Line);
        this.MarkupEditor.CaretOffset = line.Offset + Math.Min(location.Column - 1, line.Length);
        this.MarkupEditor.Select(this.MarkupEditor.CaretOffset, 0);
        this.MarkupEditor.ScrollTo(location.Line, location.Column);
        this.MarkupEditor.Focus();
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
            if (!this.isSettingsDirty) {
                this.SyncSettingsEditor();
            }
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
        // Редактирование XAML намеренно debounce'ится, но navigation уже имеет
        // готовую цель. Иначе пользователь ждёт 250 мс до начала transition.
        if (this.pendingPageTransition is not null) {
            this.RenderTimerTick(this, EventArgs.Empty);
        }
    }

    private void EditorTextChanged(object sender, EventArgs eventArgs) {
        if (ReferenceEquals(sender, this.MarkupEditor)) {
            if (this.markupEditorController.HandleTextChanged()) {
                this.isMarkupDirty = true;
                this.UpdateDocumentState();
                this.ScheduleRender();
            }

            return;
        }

        if (!this.updatingEditors) {
            if (ReferenceEquals(sender, this.ScenarioEditor)) {
                this.isScenariosDirty = true;
                this.RefreshScenarioNames();
            } else if (ReferenceEquals(sender, this.SettingsEditor)) {
                this.isSettingsDirty = true;
            }

            this.UpdateDocumentState();
            this.ScheduleRender();
        }
    }

    private void EditorPreviewKeyDown(object sender, KeyEventArgs eventArgs) {
        if (ReferenceEquals(sender, this.SettingsEditor)
            && MainWindow.IsPasteGesture(eventArgs)
            && Clipboard.ContainsText()) {
            var pastedText = Clipboard.GetText();
            var jsonText = MainWindow.EscapeWindowsPathForJson(pastedText);
            if (!string.Equals(pastedText, jsonText, StringComparison.Ordinal)) {
                this.SettingsEditor.SelectedText = jsonText;
                eventArgs.Handled = true;
                return;
            }
        }

        if (eventArgs.Key == Key.S && Keyboard.Modifiers == ModifierKeys.Control) {
            this.SaveButtonClick(this, eventArgs);
            eventArgs.Handled = true;
            return;
        }

        if (!ReferenceEquals(sender, this.MarkupEditor)) {
            return;
        }

        if (this.xamlCompletionController.HandlePreviewKeyDown(eventArgs)) {
            return;
        }
        this.markupEditorController.HandlePreviewKeyDown(eventArgs);
    }

    private static bool IsPasteGesture(KeyEventArgs eventArgs) {
        return eventArgs.Key == Key.V && Keyboard.Modifiers == ModifierKeys.Control
            || eventArgs.Key == Key.Insert && Keyboard.Modifiers == ModifierKeys.Shift;
    }

    private static string EscapeWindowsPathForJson(string value) {
        var candidate = value.Trim();
        if (candidate.Length >= 2 && candidate[0] == '"' && candidate[^1] == '"') {
            candidate = candidate[1..^1];
        }
        if (!MainWindow.IsWindowsPath(candidate)) {
            return value;
        }

        var result = new StringBuilder(value.Length * 2);
        for (var index = 0; index < value.Length; index++) {
            var character = value[index];
            if (character != '\\') {
                result.Append(character);
                continue;
            }

            result.Append("\\\\");
            if (index + 1 < value.Length && value[index + 1] == '\\') {
                index++;
            }
        }
        return result.ToString();
    }

    private static bool IsWindowsPath(string value) {
        return value.Length >= 3
            && char.IsAsciiLetter(value[0])
            && value[1] == ':'
            && value[2] == '\\'
            || value.StartsWith("\\\\", StringComparison.Ordinal);
    }

    private void RenderTimerTick(object? sender, EventArgs eventArgs) {
        this.renderTimer.Stop();
        if (this.isClosing) {
            return;
        }
        try {
            this.markupPreviewResolution = PreviewRenderer.GetPreviewResolution(this.markupEditorController.Text);
            this.ApplyPreviewLayout();
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
            string sourceMarkup = this.markupEditorController.Text;
            var locations = new Dictionary<IntPtr, (int Line, int Column)>();
            var root = PreviewRenderer.CreateRootWithLocations(sourceMarkup, data, locations);
            var previewSize = this.GetPreviewSize();
            this.animationTimer.Stop();
            var previousSession = this.previewSession;
            var transition = this.pendingPageTransition;
            this.pendingPageTransition = null;
            this.CompletePageTransition();
            PreviewSession session;
            try {
                session = new PreviewSession(
                    root,
                    this.settings.ResourcesDirectory,
                    previewSize.Width,
                    previewSize.Height,
                    previousSession is not null && transition is not null);
            }
            catch {
                NativeRuntime.xr_destroy_element(root);
                throw;
            }
            this.previewLayer.Children.Clear();
            session.SetAnimationSpeed(this.GetAnimationPlaybackRate());
            session.AnimationStarted += this.PreviewSessionAnimationStarted;
            this.animationTimer.Start();
            session.Tapped += this.PreviewSessionTapped;
            this.previewSession = session;
            if (previousSession is not null && transition is not null) {
                this.StartPageTransition(previousSession, session, transition.Value);
            } else {
                previousSession?.Dispose();
                this.previewLayer.Children.Add(session.Surface);
            }
            session.SetSourceLocations(locations);
            session.ElementSelected += (sender, location) => {
                if (this.markupEditorController.Text == sourceMarkup) {
                    this.PreviewElementSelected(sender, location);
                }
            };
            this.UpdateElementInspection();
            this.StatusText.Foreground = PreviewRenderer.ParseBrush("#8FD18B");
            this.StatusText.Text = $"Предпросмотр обновлён · {DateTime.Now:HH:mm:ss}";
            if (this.shouldRestorePreviewPosition) {
                this.shouldRestorePreviewPosition = false;
                this.Dispatcher.BeginInvoke(new Action(this.RestorePreviewPosition));
            }
        }
        catch (Exception exception) {
            this.ShowPreviewError(exception);
        }
    }

    private void AnimationTimerTick(object? sender, EventArgs eventArgs) {
        if (this.isClosing) {
            return;
        }
        bool currentAnimating = this.previewSession?.Update() ?? false;
        bool outgoingAnimating = this.outgoingSession?.Update() ?? false;
        if (!currentAnimating && !outgoingAnimating) {
            this.CompletePageTransition();
            this.animationTimer.Stop();
        }
    }

    private void PreviewSessionAnimationStarted(object? sender, EventArgs eventArgs) {
        if (this.isClosing) {
            return;
        }
        this.animationTimer.Start();
    }

    private void PreviewSessionTapped(object? sender, string elementId) {
        if (this.outgoingSession is not null || string.IsNullOrEmpty(elementId) || this.PagePicker.SelectedItem is not string pageName
            || !File.Exists(this.settings.InteractionsPath)) {
            return;
        }
        using var document = JsonDocument.Parse(File.ReadAllText(this.settings.InteractionsPath));
        var page = Path.GetFileNameWithoutExtension(pageName);
        if (!document.RootElement.TryGetProperty(page, out var actions)
            || !actions.TryGetProperty(elementId, out var element)
            || !element.TryGetProperty("tap", out var tap)
            || !tap.TryGetProperty("type", out var type)
            || type.GetString() != "navigate"
            || !tap.TryGetProperty("target", out var target)) {
            return;
        }
        // Legacy slideRight only supplies direction; effects come from XAML.
        bool backward = tap.TryGetProperty("direction", out var direction)
            ? string.Equals(direction.GetString(), "backward", StringComparison.OrdinalIgnoreCase)
            : tap.TryGetProperty("transition", out var legacy)
                && string.Equals(legacy.GetString(), "slideRight", StringComparison.OrdinalIgnoreCase);
        var targetPage = target.GetString() + ".xaml";
        if (targetPage != pageName && this.PagePicker.Items.Contains(targetPage)) {
            // Навигация выражается через тот же PagePicker, что и выбор страницы
            // пользователем. Его SelectionChanged загрузит XAML целевой страницы.
            this.pendingPageTransition = (PageId(pageName), PageId(targetPage), backward);
            this.PagePicker.SelectedItem = targetPage;
        }
    }

    private static string PageId(string fileName) {
        var name = Path.GetFileNameWithoutExtension(fileName);
        if (name.EndsWith("Page", StringComparison.Ordinal)) {
            name = name[..^4];
        }
        return name.Length == 0 ? name : char.ToLowerInvariant(name[0]) + name[1..];
    }

    private void StartPageTransition(
        PreviewSession previousSession,
        PreviewSession nextSession,
        (string From, string To, bool Backward) transition) {
        this.outgoingSession = previousSession;
        previousSession.Surface.IsHitTestVisible = false;
        nextSession.Surface.IsHitTestVisible = false;
        this.previewLayer.Children.Add(previousSession.Surface);
        this.previewLayer.Children.Add(nextSession.Surface);
        previousSession.Transition(transition.From, transition.To, transition.Backward, false);
        nextSession.Transition(transition.From, transition.To, transition.Backward, true);
        this.animationTimer.Start();
    }

    private void CompletePageTransition() {
        if (this.outgoingSession is not null) {
            this.previewLayer.Children.Remove(this.outgoingSession.Surface);
            this.outgoingSession.Dispose();
            this.outgoingSession = null;
        }
        if (this.previewSession is not null) {
            this.previewSession.Surface.IsHitTestVisible = true;
        }
    }

    private void LoadMarkup(string path) {
        this.markupPath = Path.GetFullPath(path);
        this.ConfigureMarkupWatcher();
        this.ConfigureXamlDirectoryWatcher();
        this.FilePathText.Text = this.markupPath;
        this.updatingEditors = true;
        this.markupEditorController.SetText(File.ReadAllText(this.markupPath));
        this.updatingEditors = false;
        this.isMarkupDirty = false;
        this.UpdateDocumentState();
        // PersistSettings записывает previewer.settings.json. Его изменение
        // асинхронно придёт обратно через settingsWatcher, поэтому refresh ниже
        // не должен самовольно выбирать MainPage вместо текущей страницы.
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
            if (this.ScenarioPicker.Items.Cast<string>().SequenceEqual(names)) {
                return;
            }
            this.ScenarioPicker.ItemsSource = names;
            this.ScenarioPicker.SelectedItem = names.Contains(previous) ? previous : names.FirstOrDefault();
        }
        catch (JsonException) {
        }
    }

    private void RefreshPageNames() {
        // RefreshPageNames вызывается и из settingsWatcher после PersistSettings.
        // Сохраняем выбор, чтобы такой внутренний refresh не отменял навигацию
        // MainPage -> SettingsPage через несколько сотен миллисекунд после tap.
        var previous = this.PagePicker.SelectedItem as string;
        var pages = Directory.Exists(this.settings.XamlDirectory)
            ? Directory.GetFiles(this.settings.XamlDirectory, "*.xaml", SearchOption.AllDirectories)
                .Select(path => Path.GetRelativePath(this.settings.XamlDirectory, path))
                .Order()
                .ToArray()
            : [];
        if (this.PagePicker.Items.Cast<string>().SequenceEqual(pages)) {
            return;
        }
        this.PagePicker.ItemsSource = pages;
        this.PagePicker.SelectedItem = pages.Contains(previous)
            ? previous
            : pages.Contains("MainPage.xaml")
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

    private void ConfigureXamlDirectoryWatcher() {
        this.xamlDirectoryWatcher?.Dispose();
        if (!Directory.Exists(this.settings.XamlDirectory)) {
            this.xamlDirectoryWatcher = null;
            return;
        }
        this.xamlDirectoryWatcher = new FileSystemWatcher(this.settings.XamlDirectory, "*.xaml") {
            IncludeSubdirectories = true,
            NotifyFilter = NotifyFilters.FileName | NotifyFilters.DirectoryName,
            EnableRaisingEvents = true,
        };
        this.xamlDirectoryWatcher.Created += this.ExternalFileChanged;
        this.xamlDirectoryWatcher.Deleted += this.ExternalFileChanged;
        this.xamlDirectoryWatcher.Renamed += this.ExternalFileRenamed;
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
        if (this.isClosing) {
            return;
        }
        this.externalRefreshTimer.Stop();
        this.externalRefreshTimer.Start();
    }

    private void ExternalRefreshTimerTick(object? sender, EventArgs eventArgs) {
        this.externalRefreshTimer.Stop();
        if (this.isClosing) {
            return;
        }
        try {
            bool previewChanged = false;
            this.RefreshPageNames();
            if (this.markupPath is not null && File.Exists(this.markupPath)) {
                var markup = File.ReadAllText(this.markupPath);
                if (!this.isMarkupDirty
                    && !string.Equals(markup, this.markupEditorController.Text, StringComparison.Ordinal)) {
                    this.updatingEditors = true;
                    this.markupEditorController.SetText(markup);
                    this.updatingEditors = false;
                    this.isMarkupDirty = false;
                    this.UpdateDocumentState();
                    previewChanged = true;
                }
            }
            if (File.Exists(this.settings.ScenariosPath)) {
                var scenarios = File.ReadAllText(this.settings.ScenariosPath);
                if (!this.isScenariosDirty
                    && !string.Equals(scenarios, this.ScenarioEditor.Text, StringComparison.Ordinal)) {
                    this.updatingEditors = true;
                    this.ScenarioEditor.Text = scenarios;
                    this.updatingEditors = false;
                    this.isScenariosDirty = false;
                    this.UpdateDocumentState();
                    this.RefreshScenarioNames();
                    previewChanged = true;
                }
            }
            if (File.Exists(this.settings.FilePath)) {
                var settingsJson = File.ReadAllText(this.settings.FilePath);
                if (!this.isSettingsDirty
                    && !string.Equals(settingsJson, this.SettingsEditor.Text, StringComparison.Ordinal)) {
                    this.settings = PreviewerSettings.Parse(settingsJson, this.settings.FilePath);
                    this.updatingEditors = true;
                    this.SettingsEditor.Text = settingsJson;
                    this.updatingEditors = false;
                    this.isSettingsDirty = false;
                    this.UpdateDocumentState();
                    this.ConfigureWatchers();
                    // Не сбрасывает PagePicker на MainPage: RefreshPageNames
                    // восстанавливает страницу, выбранную обработчиком navigation.
                    this.RefreshPageNames();
                    this.ConfigureMouseWheelScrolling();
                    this.ApplyEditorScale();
                    this.ApplySettingsToPreviewControls();
                    previewChanged = true;
                }
            }
            if (previewChanged) {
                this.ScheduleRender();
            }
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
        // Синхронизируем отображаемый JSON после собственного сохранения.
        // Тогда settingsWatcher не принимает нашу же запись за внешнее
        // изменение и не запускает лишний тяжёлый render PreviewSession.
        if (!this.isSettingsDirty) {
            this.updatingEditors = true;
            this.SettingsEditor.Text = this.settings.ToJson();
            this.updatingEditors = false;
        }
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
        } else {
            // Положение из прошлой сессии не восстанавливаем: окно должно
            // открываться в центре рабочей области текущего экрана.
            var workArea = SystemParameters.WorkArea;
            this.Left = workArea.Left + (workArea.Width - this.Width) / 2.0;
            this.Top = workArea.Top + (workArea.Height - this.Height) / 2.0;
        }
        if (this.settings.EditorPaneWidth > 0.0) {
            this.EditorColumn.Width = new GridLength(this.settings.EditorPaneWidth, GridUnitType.Pixel);
        }
    }

    private void WindowClosing(object? sender, System.ComponentModel.CancelEventArgs eventArgs) {
        if (this.isClosing) {
            return;
        }
        this.isClosing = true;
        this.settingsPersistenceReady = false;
        this.renderTimer.Stop();
        this.externalRefreshTimer.Stop();
        this.animationTimer.Stop();
        this.previewZoomTimer.Stop();
        this.smoothScrollTimer.Stop();
        this.pendingPageTransition = null;
        this.markupEditorController.Dispose();
        var session = this.previewSession;
        this.previewSession = null;
        this.CompletePageTransition();
        session?.Dispose();
        this.previewLayer.Children.Clear();
        this.markupWatcher?.Dispose();
        this.xamlDirectoryWatcher?.Dispose();
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
        this.UpdateElementInspection();
        this.MarkupEditor.Visibility = this.editorMode == EditorMode.Xaml ? Visibility.Visible : Visibility.Collapsed;
        this.ScenarioPanel.Visibility = this.editorMode == EditorMode.Scenarios ? Visibility.Visible : Visibility.Collapsed;
        this.SettingsPanel.Visibility = this.editorMode == EditorMode.Settings ? Visibility.Visible : Visibility.Collapsed;
        this.OpenButton.IsEnabled = this.editorMode == EditorMode.Xaml;
        this.XamlModeText.Foreground = PreviewRenderer.ParseBrush(
            this.editorMode == EditorMode.Xaml ? "#D5BD7D" : "#E6E6E6");
        this.ScenariosModeText.Foreground = PreviewRenderer.ParseBrush(
            this.editorMode == EditorMode.Scenarios ? "#D5BD7D" : "#E6E6E6");
        this.UpdateDocumentState();
    }

    private void UpdateDocumentState() {
        this.XamlModeText.Text = this.isMarkupDirty ? "XAML *" : "XAML";
        this.ScenariosModeText.Text = this.isScenariosDirty ? "Сценарии *" : "Сценарии";
        this.SettingsButton.Content = this.isSettingsDirty ? "Настройки *" : "Настройки";
        this.SaveButton.IsEnabled = this.editorMode switch {
            EditorMode.Xaml => this.isMarkupDirty,
            EditorMode.Scenarios => this.isScenariosDirty,
            EditorMode.Settings => this.isSettingsDirty,
            _ => false,
        };
    }

    private void InitializePreviewControls() {
        this.ApplySettingsToPreviewControls();
        this.ApplyPreviewLayout();
    }

    private void ApplySettingsToPreviewControls() {
        this.updatingPreviewControls = true;
        try {
            this.DevicePresetPicker.ItemsSource = this.settings.PreviewResolutions;
            this.DevicePresetPicker.SelectedItem = this.settings.PreviewResolutions.FirstOrDefault(
                preset => preset.Width == this.settings.PreviewWidth
                    && preset.Height == this.settings.PreviewHeight);
            this.PreviewOrientationToggle.IsChecked = this.settings.IsPreviewLandscape;
            var speeds = this.settings.AnimationPlaybackRates.Select(rate => new AnimationSpeed {
                Name = rate.ToString("G", System.Globalization.CultureInfo.InvariantCulture) + "×",
                Rate = rate,
            }).ToArray();
            this.AnimationSpeedPicker.ItemsSource = speeds;
            this.AnimationSpeedPicker.SelectedItem = speeds.FirstOrDefault(
                speed => speed.Rate == this.GetAnimationPlaybackRate());
        }
        finally {
            this.updatingPreviewControls = false;
        }
        this.UpdatePreviewOrientationToggle();
        this.ApplyPreviewLayout();
        this.previewSession?.SetAnimationSpeed(this.GetAnimationPlaybackRate());
        this.outgoingSession?.SetAnimationSpeed(this.GetAnimationPlaybackRate());
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

    private double GetAnimationPlaybackRate() {
        return this.settings.AnimationPlaybackRate;
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
        if (this.markupPreviewResolution is { } resolution) {
            return resolution;
        }
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

    private void PreviewViewportScrollChanged(object sender, ScrollChangedEventArgs eventArgs) {
        if (eventArgs.HorizontalChange == 0.0 && eventArgs.VerticalChange == 0.0) {
            return;
        }
        this.settings.PreviewHorizontalOffset = this.PreviewViewport.HorizontalOffset;
        this.settings.PreviewVerticalOffset = this.PreviewViewport.VerticalOffset;
        this.PersistSettings();
    }

    private void RestorePreviewPosition() {
        this.PreviewViewport.ScrollToHorizontalOffset(this.settings.PreviewHorizontalOffset);
        this.PreviewViewport.ScrollToVerticalOffset(this.settings.PreviewVerticalOffset);
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
        this.isSettingsDirty = false;
        this.UpdateDocumentState();
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
                StartVerticalOffset = scrollViewer.VerticalOffset,
                TargetVerticalOffset = scrollViewer.VerticalOffset,
                LastAppliedVerticalOffset = scrollViewer.VerticalOffset,
            };
            this.smoothScrollStates[editor] = state;
        }

        var currentOffset = scrollViewer.VerticalOffset;
        if (Math.Abs(currentOffset - state.LastAppliedVerticalOffset) > 0.5) {
            state.StartVerticalOffset = currentOffset;
            state.TargetVerticalOffset = currentOffset;
            state.LastAppliedVerticalOffset = currentOffset;
            state.IsAnimating = false;
        }

        var steps = Math.Max(1, Math.Abs(eventArgs.Delta) / Mouse.MouseWheelDeltaForOneLine);
        var lineHeight = editor.TextArea.TextView.DefaultLineHeight;
        var offset = steps * this.settings.MouseWheelLines * lineHeight;
        state.TargetVerticalOffset = Math.Clamp(
            state.TargetVerticalOffset + (eventArgs.Delta > 0 ? -offset : offset),
            0.0,
            scrollViewer.ScrollableHeight);
        state.StartVerticalOffset = currentOffset;
        state.LastAppliedVerticalOffset = currentOffset;
        state.AnimationStartedAt = Stopwatch.GetTimestamp();
        state.AnimationDuration = TimeSpan.FromMilliseconds(Math.Clamp(
            this.settings.MouseWheelAnimationDurationMilliseconds,
            50.0,
            5000.0));
        state.SmoothingMode = this.GetScrollSmoothingMode();
        if (state.SmoothingMode == ScrollSmoothingMode.None) {
            scrollViewer.ScrollToVerticalOffset(state.TargetVerticalOffset);
            state.LastAppliedVerticalOffset = state.TargetVerticalOffset;
            state.IsAnimating = false;
            eventArgs.Handled = true;
            return;
        }

        state.IsAnimating = Math.Abs(state.TargetVerticalOffset - currentOffset) > 0.5;
        this.smoothScrollTimer.Start();
        eventArgs.Handled = true;
    }

    private void SmoothScrollTimerTick(object? sender, EventArgs eventArgs) {
        var isAnimating = false;
        foreach (var state in this.smoothScrollStates.Values) {
            if (!state.IsAnimating) {
                continue;
            }

            var elapsed = Stopwatch.GetElapsedTime(state.AnimationStartedAt);
            var progress = Math.Clamp(elapsed / state.AnimationDuration, 0.0, 1.0);
            var interpolatedProgress = MainWindow.InterpolateScrollProgress(state.SmoothingMode, progress);
            var nextOffset = state.StartVerticalOffset
                + (state.TargetVerticalOffset - state.StartVerticalOffset) * interpolatedProgress;
            state.ScrollViewer.ScrollToVerticalOffset(nextOffset);
            state.LastAppliedVerticalOffset = nextOffset;
            if (progress >= 1.0) {
                state.ScrollViewer.ScrollToVerticalOffset(state.TargetVerticalOffset);
                state.LastAppliedVerticalOffset = state.TargetVerticalOffset;
                state.IsAnimating = false;
                continue;
            }

            isAnimating = true;
        }
        if (!isAnimating) {
            this.smoothScrollTimer.Stop();
        }
    }

    private ScrollSmoothingMode GetScrollSmoothingMode() {
        return Enum.TryParse<ScrollSmoothingMode>(
            this.settings.MouseWheelSmoothingMode,
            true,
            out var smoothingMode)
            ? smoothingMode
            : ScrollSmoothingMode.Exponential;
    }

    private static double InterpolateScrollProgress(ScrollSmoothingMode smoothingMode, double progress) {
        return smoothingMode switch {
            ScrollSmoothingMode.Linear => progress,
            ScrollSmoothingMode.Smoothstep => progress * progress * (3.0 - 2.0 * progress),
            ScrollSmoothingMode.Smootherstep => progress * progress * progress
                * (progress * (progress * 6.0 - 15.0) + 10.0),
            ScrollSmoothingMode.EaseOutCubic => 1.0 - Math.Pow(1.0 - progress, 3.0),
            ScrollSmoothingMode.Exponential => (1.0 - Math.Exp(-6.0 * progress))
                / (1.0 - Math.Exp(-6.0)),
            _ => progress,
        };
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
        if (this.isClosing) {
            return;
        }
        this.renderTimer.Stop();
        this.renderTimer.Start();
    }
}