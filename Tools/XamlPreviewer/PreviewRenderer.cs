using System.Globalization;
using System.IO;
using System.Text.Json;
using System.Windows;
using System.Windows.Media;
using System.Xml.Linq;

namespace XamlPreviewer;

internal static class PreviewRenderer {
    public static FrameworkElement Render(string markup, JsonElement data) {
        var document = XDocument.Parse(markup, LoadOptions.SetLineInfo);
        MarkupValidator.Validate(document);
        var rootNode = document.Root
            ?? throw new InvalidDataException("Разметка не содержит корневого элемента.");
        var root = PreviewRenderer.Build(rootNode, data);
        try {
            return AnglePreviewRenderer.Render(root);
        }
        finally {
            NativeRuntime.xr_destroy_element(root);
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