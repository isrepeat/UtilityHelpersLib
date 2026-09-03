using System.IO;
using System.Text.Encodings.Web;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace XamlPreviewer;

internal sealed class PreviewerSettings {
    private const string DebugPreviewDirectory = @"C:\WORK\TEST\XamlPreviewer";
    private const string DefaultScenarios = """
        {
          "MainPage": {
            "Будильники": {
              "PackageVersion": "1.4.0",
              "ClockText": "через 6 ч 35 мин",
              "Alarms": []
            }
          },
          "SettingsPage": {
            "Основной": {
              "PackageVersion": "1.4.0",
              "Theme": "Тёмная",
              "Sound": "Мелодия по умолчанию"
            }
          }
        }
        """;
    private static readonly JsonSerializerOptions JsonOptions = new() {
        Encoder = JavaScriptEncoder.UnsafeRelaxedJsonEscaping,
        WriteIndented = true
    };

    public required string XamlDirectory { get; init; }
    public required string ScenariosPath { get; init; }
    public required string ResourcesDirectory { get; init; }
    public string? LastMarkupPath { get; set; }
    public string? LastScenarioName { get; set; }
    public double WindowWidth { get; set; }
    public double WindowHeight { get; set; }
    public bool IsMaximized { get; set; }
    public double EditorPaneWidth { get; set; }
    public double EditorScale { get; set; } = 1.0;
    public int MouseWheelLines { get; set; } = 8;
    public int PreviewWidth { get; set; } = 720;
    public int PreviewHeight { get; set; } = 1600;
    public double PreviewScale { get; set; }
    public bool IsPreviewLandscape { get; set; }

    [JsonIgnore]
    public string FilePath { get; private set; } = string.Empty;

    public static PreviewerSettings LoadDebug() {
        var settingsPath = Path.Combine(AppContext.BaseDirectory, "previewer.settings.json");
        Directory.CreateDirectory(DebugPreviewDirectory);
        PreviewerSettings settings;
        if (File.Exists(settingsPath)) {
            settings = JsonSerializer.Deserialize<PreviewerSettings>(File.ReadAllText(settingsPath))
                ?? CreateDefaults(settingsPath);
            settings.FilePath = settingsPath;
        } else {
            settings = CreateDefaults(settingsPath);
            settings.Save();
        }
        settings.CreateDefaultScenariosIfMissing();
        return settings;
    }

    public static PreviewerSettings Parse(string json, string filePath) {
        var settings = JsonSerializer.Deserialize<PreviewerSettings>(json)
            ?? throw new InvalidDataException("Настройки не содержат объект.");
        settings.FilePath = filePath;
        return settings;
    }

    public void Save() {
        File.WriteAllText(this.FilePath, this.ToJson());
    }

    public string ToJson() {
        return JsonSerializer.Serialize(this, JsonOptions).TrimEnd();
    }

    private static PreviewerSettings CreateDefaults(string settingsPath) {
        return new PreviewerSettings {
            FilePath = settingsPath,
            XamlDirectory = DebugPreviewDirectory,
            ScenariosPath = Path.Combine(DebugPreviewDirectory, "scenarios.json"),
            ResourcesDirectory = DebugPreviewDirectory,
        };
    }

    private void CreateDefaultScenariosIfMissing() {
        if (File.Exists(this.ScenariosPath)) {
            return;
        }
        var directory = Path.GetDirectoryName(this.ScenariosPath);
        if (!string.IsNullOrEmpty(directory)) {
            Directory.CreateDirectory(directory);
        }
        File.WriteAllText(this.ScenariosPath, DefaultScenarios.TrimEnd());
    }
}