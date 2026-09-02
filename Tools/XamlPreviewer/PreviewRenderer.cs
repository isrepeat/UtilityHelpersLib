using System.Globalization;
using System.IO;
using System.Text.Json;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Xml.Linq;

namespace XamlPreviewer;

internal static class PreviewRenderer {
    public static FrameworkElement Render(string markup, JsonElement data, string? markupPath) {
        var document = XDocument.Parse(markup, LoadOptions.SetLineInfo);
        MarkupValidator.Validate(document);
        var rootNode = document.Root
            ?? throw new InvalidDataException("Разметка не содержит корневого элемента.");
        var root = PreviewRenderer.Build(rootNode, data);
        try {
            NativeRuntime.Ensure(NativeRuntime.xr_layout(root, 720, 1280) != 0);
            return new NativeRenderSurface(root, markupPath);
        }
        catch {
            NativeRuntime.xr_destroy_element(root);
            throw;
        }
    }

    public static SolidColorBrush ParseBrush(string value) {
        return new SolidColorBrush((Color)ColorConverter.ConvertFromString(value));
    }

    private static IntPtr Build(XElement node, JsonElement data) {
        var element = NativeRuntime.xr_create_element(node.Name.LocalName);
        if (element == IntPtr.Zero) {
            throw new InvalidOperationException(NativeRuntime.GetLastError());
        }

        try {
            foreach (var attribute in node.Attributes()) {
                var value = PreviewRenderer.Resolve(attribute.Value, data);
                PreviewRenderer.SetAttribute(element, attribute.Name.LocalName, value);
            }

            PreviewRenderer.ApplyDefinitions(element, node);
            if (node.Name.LocalName == "ListView") {
                PreviewRenderer.BuildListViewItems(element, node, data);
            }
            else {
                foreach (var childNode in node.Elements().Where(PreviewRenderer.IsVisualElement)) {
                    PreviewRenderer.AddChild(element, PreviewRenderer.Build(childNode, data));
                }
            }

            return element;
        }
        catch {
            NativeRuntime.xr_destroy_element(element);
            throw;
        }
    }

    private static void BuildListViewItems(IntPtr listView, XElement node, JsonElement data) {
        var source = PreviewRenderer.ResolveElement(PreviewRenderer.Attribute(node, "itemsSource"), data);
        var template = node.Elements()
            .FirstOrDefault(element => element.Name.LocalName == "ListView.ItemTemplate")?
            .Elements()
            .SingleOrDefault(element => element.Name.LocalName == "DataTemplate")?
            .Elements()
            .SingleOrDefault();
        if (source is not JsonElement items || items.ValueKind != JsonValueKind.Array || template is null) {
            return;
        }

        foreach (var item in items.EnumerateArray()) {
            PreviewRenderer.AddChild(listView, PreviewRenderer.Build(template, item));
        }
    }

    private static void ApplyDefinitions(IntPtr element, XElement node) {
        var columns = node.Elements().FirstOrDefault(child => child.Name.LocalName == "columnDefinitions");
        if (columns is not null) {
            PreviewRenderer.SetAttribute(element, "columns", string.Join(",", columns.Elements().Select(
                definition => PreviewRenderer.Attribute(definition, "width") ?? "*")));
        }

        var rows = node.Elements().FirstOrDefault(child => child.Name.LocalName == "rowDefinitions");
        if (rows is not null) {
            PreviewRenderer.SetAttribute(element, "rows", string.Join(",", rows.Elements().Select(
                definition => PreviewRenderer.Attribute(definition, "height") ?? "*")));
        }
    }

    private static void AddChild(IntPtr parent, IntPtr child) {
        if (NativeRuntime.xr_add_child(parent, child) == 0) {
            NativeRuntime.xr_destroy_element(child);
            throw new InvalidOperationException(NativeRuntime.GetLastError());
        }
    }

    private static void SetAttribute(IntPtr element, string name, string value) {
        if (NativeRuntime.xr_set_attribute(element, name, value) == 0) {
            throw new InvalidOperationException($"{name}: {NativeRuntime.GetLastError()}");
        }
    }

    private static string Resolve(string value, JsonElement data) {
        var resolved = PreviewRenderer.ResolveElement(value, data);
        return resolved switch {
            null => string.Empty,
            bool boolean => boolean ? "True" : "False",
            JsonElement element => element.ToString(),
            _ => Convert.ToString(resolved, CultureInfo.InvariantCulture) ?? string.Empty
        };
    }

    private static object? ResolveElement(string? value, JsonElement data) {
        if (value is null || !value.StartsWith("{Binding ", StringComparison.Ordinal)
            || !value.EndsWith('}')) {
            return value;
        }

        var path = value[9..^1].Split(',', 2)[0].Trim();
        var current = data;
        foreach (var segment in path.Split('.')) {
            if (current.ValueKind != JsonValueKind.Object || !current.TryGetProperty(segment, out current)) {
                return $"{{{path}?}}";
            }
        }

        return current.ValueKind switch {
            JsonValueKind.String => current.GetString(),
            JsonValueKind.True => true,
            JsonValueKind.False => false,
            JsonValueKind.Number when current.TryGetInt64(out var integer) => integer,
            JsonValueKind.Number => current.GetDouble(),
            JsonValueKind.Null => null,
            _ => current
        };
    }

    private static bool IsVisualElement(XElement element) {
        return element.Name.LocalName != "columnDefinitions"
            && element.Name.LocalName != "rowDefinitions"
            && !element.Name.LocalName.Contains('.');
    }

    private static string? Attribute(XElement element, string name) {
        return element.Attributes().FirstOrDefault(attribute => string.Equals(
            attribute.Name.LocalName,
            name,
            StringComparison.OrdinalIgnoreCase))?.Value;
    }
}

internal sealed class NativeRenderSurface : FrameworkElement {
    private readonly string? markupPath;
    private IntPtr root;

    public NativeRenderSurface(IntPtr root, string? markupPath) {
        this.root = root;
        this.markupPath = markupPath;
        this.Width = 720;
        this.Height = 1280;
    }

    ~NativeRenderSurface() {
        this.ReleaseRoot();
    }

    protected override void OnRender(DrawingContext drawingContext) {
        base.OnRender(drawingContext);
        var count = NativeRuntime.xr_render(this.root, null, 0);
        if (count < 0) {
            throw new InvalidOperationException(NativeRuntime.GetLastError());
        }

        var commands = new NativeCommand[count];
        NativeRuntime.xr_render(this.root, commands, commands.Length);
        foreach (var command in commands) {
            this.DrawCommand(drawingContext, command);
        }
    }

    private void DrawCommand(DrawingContext drawingContext, NativeCommand command) {
        var type = (NativeCommandType)command.Type;
        var bounds = new Rect(
            command.Bounds.X,
            command.Bounds.Y,
            command.Bounds.Width,
            command.Bounds.Height);
        var brush = new SolidColorBrush(Color.FromArgb(
            NativeRenderSurface.ToByte(command.Color.Alpha),
            NativeRenderSurface.ToByte(command.Color.Red),
            NativeRenderSurface.ToByte(command.Color.Green),
            NativeRenderSurface.ToByte(command.Color.Blue)));
        if (type == NativeCommandType.BeginClip) {
            drawingContext.PushClip(new RectangleGeometry(bounds));
        }
        else if (type == NativeCommandType.EndClip) {
            drawingContext.Pop();
        }
        else if (type == NativeCommandType.Outline) {
            drawingContext.DrawRectangle(null, new Pen(brush, 2), bounds);
        }
        else if (type == NativeCommandType.RoundedRect) {
            drawingContext.DrawRoundedRectangle(brush, null, bounds, command.Value, command.Value);
        }
        else if (type == NativeCommandType.RoundedRectOutline) {
            drawingContext.DrawRoundedRectangle(
                null,
                new Pen(brush, command.GetAuxiliaryFloat()),
                bounds,
                command.Value,
                command.Value);
        }
        else if (type == NativeCommandType.Text) {
            this.DrawText(drawingContext, command, bounds, brush);
        }
        else if (type == NativeCommandType.Image) {
            this.DrawImage(drawingContext, command.GetText(), bounds);
        }
    }

    private void DrawText(
        DrawingContext drawingContext,
        NativeCommand command,
        Rect bounds,
        Brush brush) {
        var typeface = new Typeface(
            NativeRenderSurface.NativeFont(this.markupPath),
            FontStyles.Normal,
            command.GetAuxiliary() is "Bold" or "SemiBold" ? FontWeights.Bold : FontWeights.Normal,
            FontStretches.Normal);
        var text = new FormattedText(
            command.GetText(),
            CultureInfo.CurrentUICulture,
            FlowDirection.LeftToRight,
            typeface,
            command.Value,
            brush,
            VisualTreeHelper.GetDpi(this).PixelsPerDip);
        drawingContext.DrawText(
            text,
            new Point(
                bounds.X + (bounds.Width - text.Width) / 2,
                bounds.Y + (bounds.Height - text.Height) / 2));
    }

    private void DrawImage(DrawingContext drawingContext, string source, Rect bounds) {
        if (this.markupPath is null) {
            return;
        }

        var path = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(this.markupPath)!, source));
        if (File.Exists(path)) {
            drawingContext.DrawImage(new BitmapImage(new Uri(path)), bounds);
        }
    }

    private void ReleaseRoot() {
        if (this.root != IntPtr.Zero) {
            NativeRuntime.xr_destroy_element(this.root);
            this.root = IntPtr.Zero;
        }
    }

    private static byte ToByte(float value) {
        return (byte)Math.Clamp(Math.Round(value * 255), byte.MinValue, byte.MaxValue);
    }

    private static FontFamily NativeFont(string? markupPath) {
        if (markupPath is not null) {
            var directory = new DirectoryInfo(Path.GetDirectoryName(markupPath)!);
            while (directory is not null) {
                var fontPath = Path.Combine(directory.FullName, "app", "src", "main", "assets", "Roboto-Regular.ttf");
                if (File.Exists(fontPath)) {
                    return new FontFamily(
                        new Uri(Path.GetDirectoryName(fontPath) + Path.DirectorySeparatorChar),
                        "./#Roboto");
                }

                directory = directory.Parent;
            }
        }

        return new FontFamily("Roboto");
    }
}