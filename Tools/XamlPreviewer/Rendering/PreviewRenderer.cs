using System.Globalization;
using System.IO;
using System.Text.Json;
using System.Windows;
using System.Windows.Media;
using System.Xml.Linq;

namespace XamlPreviewer;

internal static class PreviewRenderer {
    public static (int Width, int Height)? GetPreviewResolution(string markup) {
        var document = XDocument.Parse(markup, LoadOptions.None);
        var instruction = document.Nodes().OfType<XProcessingInstruction>()
            .FirstOrDefault(node => node.Target == "mobileclock-preview");
        if (instruction is null) {
            return null;
        }
        var attributes = XElement.Parse($"<preview {instruction.Data} />");
        if (!int.TryParse(attributes.Attribute("width")?.Value, out var width)
            || !int.TryParse(attributes.Attribute("height")?.Value, out var height)
            || width <= 0 || height <= 0) {
            throw new InvalidDataException("mobileclock-preview требует положительные width и height.");
        }
        return (width, height);
    }

    public static IntPtr CreateRoot(string markup, JsonElement data) {
        var document = XDocument.Parse(markup, LoadOptions.SetLineInfo);
        MarkupValidator.Validate(document);
        var rootNode = document.Root
            ?? throw new InvalidDataException("Разметка не содержит корневого элемента.");
        return PreviewRenderer.Build(rootNode, data);
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
                if (attribute.IsNamespaceDeclaration) {
                    continue;
                }
                var value = PreviewRenderer.Resolve(attribute.Value, data);
                PreviewRenderer.SetAttribute(element, attribute.Name.LocalName, value);
            }

            PreviewRenderer.ApplyDefinitions(element, node);
            PreviewRenderer.ApplyStoryboards(element, node);
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

    private static void ApplyStoryboards(IntPtr element, XElement node) {
        var collection = node.Elements().FirstOrDefault(child => child.Name.LocalName
            == $"{node.Name.LocalName}.Storyboards");
        if (collection is null) {
            return;
        }

        foreach (var storyboard in collection.Elements().Where(child => child.Name.LocalName == "Storyboard")) {
            var trigger = PreviewRenderer.ParseTrigger(PreviewRenderer.Attribute(storyboard, "trigger"));
            foreach (var track in storyboard.Elements()) {
                var property = track.Name.LocalName == "RendererAnimation"
                    && (PreviewRenderer.Attribute(track, "name") == "RippleWave"
                        || PreviewRenderer.Attribute(track, "name") == "SoftPulse")
                    ? 4 : PreviewRenderer.ParseProperty(PreviewRenderer.Attribute(track, "property"));
                var from = PreviewRenderer.ParseAnimationValue(PreviewRenderer.Attribute(track, "from"), "Current");
                var to = PreviewRenderer.ParseAnimationValue(PreviewRenderer.Attribute(track, "to"), "ToggleState");
                if (!int.TryParse(PreviewRenderer.Attribute(track, "duration"), out var duration)
                    || duration < 0) {
                    throw new InvalidDataException("Animation track requires non-negative duration.");
                }

                var easing = PreviewRenderer.Attribute(track, "easing") switch {
                    null or "CubicOut" => 1,
                    "Linear" => 0,
                    _ => throw new InvalidDataException("Animation easing must be Linear or CubicOut."),
                };
                var intensity = PreviewRenderer.ParseEffectParameter(track, "intensity", 0.45f, 0.0f);
                var spread = PreviewRenderer.ParseEffectParameter(track, "spread", 0.28f, float.Epsilon);
                var fadeExponent = PreviewRenderer.ParseEffectParameter(track, "fadeExponent", 2.0f, float.Epsilon);
                NativeRuntime.Ensure(NativeRuntime.xr_add_storyboard_track(
                    element, trigger, property, from, to, duration, easing,
                    intensity, spread, fadeExponent) != 0);
            }
        }
    }

    private static int ParseTrigger(string? value) {
        return value switch {
            "PointerDown" => 0,
            "PointerUp" => 1,
            "Toggled" => 2,
            _ => throw new InvalidDataException("Storyboard trigger must be PointerDown, PointerUp or Toggled."),
        };
    }

    private static int ParseProperty(string? value) {
        return value switch {
            "opacity" => 0,
            "renderOffsetX" => 1,
            "toggleProgress" => 2,
            "pressProgress" => 3,
            "waveProgress" => 4,
            _ => throw new InvalidDataException("Unsupported FloatAnimation property."),
        };
    }

    private static float ParseAnimationValue(string? value, string stateValue) {
        return value == stateValue
            ? float.NaN
            : float.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out var result)
                ? result
                : throw new InvalidDataException("Animation value must be a number or supported state value.");
    }

    private static float ParseEffectParameter(XElement element, string name, float defaultValue, float exclusiveMinimum) {
        var value = PreviewRenderer.Attribute(element, name);
        if (value is null) {
            return defaultValue;
        }

        if (!float.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out var result)
            || result < exclusiveMinimum || (exclusiveMinimum > 0.0f && result == exclusiveMinimum)) {
            throw new InvalidDataException($"{name} has an invalid value.");
        }
        return result;
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