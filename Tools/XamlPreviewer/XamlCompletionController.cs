using ICSharpCode.AvalonEdit;
using ICSharpCode.AvalonEdit.CodeCompletion;
using ICSharpCode.AvalonEdit.Document;
using ICSharpCode.AvalonEdit.Editing;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;

namespace XamlPreviewer;

internal sealed class XamlCompletionController {
    private static readonly string[] ElementNames = [
        "Page",
        "StackPanel",
        "Grid",
        "Border",
        "TextBlock",
        "Button",
        "IconButton",
        "ToggleSwitch",
        "ScrollViewer",
        "Image",
        "SvgImage",
        "ListView",
        "ListView.ItemTemplate",
        "DataTemplate",
        "columnDefinitions",
        "columnDefinition",
        "rowDefinitions",
        "rowDefinition",
    ];

    private static readonly string[] CommonAttributes = [
        "id",
        "horizontalAlignment",
        "verticalAlignment",
        "gridColumn",
        "gridRow",
        "width",
        "height",
        "margin",
        "visibility",
        "isEnabled",
        "opacity",
    ];

    private readonly TextEditor editor;
    private CompletionWindow? completionWindow;

    public XamlCompletionController(TextEditor editor) {
        this.editor = editor;
        this.editor.TextArea.TextEntered += this.TextAreaTextEntered;
    }

    public bool HandlePreviewKeyDown(KeyEventArgs eventArgs) {
        if (eventArgs.Key != Key.Enter || this.completionWindow is null) {
            return false;
        }

        this.completionWindow.CompletionList.RequestInsertion(eventArgs);
        eventArgs.Handled = true;
        return true;
    }

    private void TextAreaTextEntered(object? sender, TextCompositionEventArgs eventArgs) {
        if (eventArgs.Text == "<") {
            this.ShowCompletion(XamlCompletionController.ElementNames.Select(
                name => new XamlCompletionData(name, "Элемент XAML")));
            return;
        }

        if (eventArgs.Text == " " && this.TryGetOpenElementName(out var elementName)) {
            this.ShowCompletion(XamlCompletionController.GetAttributes(elementName).Select(
                name => new XamlCompletionData($"{name}=\"\"", "Атрибут XAML", name.Length + 2)));
            return;
        }

        if (eventArgs.Text == ">") {
            this.InsertClosingTag();
        }
    }

    private void ShowCompletion(IEnumerable<XamlCompletionData> completionData) {
        this.completionWindow?.Close();
        var window = new CompletionWindow(this.editor.TextArea) {
            Background = PreviewRenderer.ParseBrush("#252525"),
            BorderBrush = PreviewRenderer.ParseBrush("#4A4A4A"),
            Foreground = PreviewRenderer.ParseBrush("#E6E6E6"),
            MaxHeight = 520,
            MinWidth = 360,
            ResizeMode = ResizeMode.NoResize,
            SizeToContent = SizeToContent.Height,
            Width = 360,
        };
        window.Resources[SystemColors.ActiveBorderBrushKey] = PreviewRenderer.ParseBrush("#4A4A4A");
        window.Resources[SystemColors.InactiveBorderBrushKey] = PreviewRenderer.ParseBrush("#4A4A4A");
        window.Resources[SystemColors.ControlBrushKey] = PreviewRenderer.ParseBrush("#252525");
        window.Resources[SystemColors.ControlTextBrushKey] = PreviewRenderer.ParseBrush("#E6E6E6");
        window.Resources[SystemColors.HighlightBrushKey] = PreviewRenderer.ParseBrush("#5A4D26");
        window.Resources[SystemColors.HighlightTextBrushKey] = PreviewRenderer.ParseBrush("#FFFFFF");
        window.Resources[SystemColors.WindowBrushKey] = PreviewRenderer.ParseBrush("#252525");
        window.Resources[SystemColors.WindowTextBrushKey] = PreviewRenderer.ParseBrush("#E6E6E6");
        window.CompletionList.MinHeight = 0;
        window.CompletionList.Template = this.CreateCompletionListTemplate();
        foreach (var item in completionData) {
            window.CompletionList.CompletionData.Add(item);
        }
        window.Closed += (_, _) => {
            if (ReferenceEquals(this.completionWindow, window)) {
                this.completionWindow = null;
            }
        };
        window.Loaded += (_, _) => this.ApplyCompletionTheme(window);
        this.completionWindow = window;
        window.Show();
    }

    private void ApplyCompletionTheme(CompletionWindow window) {
        var listBox = window.CompletionList.ListBox;
        listBox.Background = PreviewRenderer.ParseBrush("#252525");
        listBox.BorderBrush = PreviewRenderer.ParseBrush("#626262");
        listBox.BorderThickness = new Thickness(0);
        listBox.Foreground = PreviewRenderer.ParseBrush("#E6E6E6");
        listBox.HorizontalContentAlignment = HorizontalAlignment.Stretch;
        var itemStyle = new Style(typeof(ListBoxItem));
        itemStyle.Setters.Add(new Setter(Control.BackgroundProperty, Brushes.Transparent));
        itemStyle.Setters.Add(new Setter(Control.ForegroundProperty, PreviewRenderer.ParseBrush("#E6E6E6")));
        itemStyle.Setters.Add(new Setter(Control.PaddingProperty, new Thickness(7, 4, 7, 4)));
        var selectedTrigger = new Trigger {
            Property = ListBoxItem.IsSelectedProperty,
            Value = true,
        };
        selectedTrigger.Setters.Add(new Setter(Control.BackgroundProperty, PreviewRenderer.ParseBrush("#5A4D26")));
        selectedTrigger.Setters.Add(new Setter(Control.ForegroundProperty, PreviewRenderer.ParseBrush("#FFF3BF")));
        itemStyle.Triggers.Add(selectedTrigger);
        listBox.ItemContainerStyle = itemStyle;
    }

    private ControlTemplate CreateCompletionListTemplate() {
        var template = new ControlTemplate(typeof(CompletionList));
        var border = new FrameworkElementFactory(typeof(Border));
        border.SetValue(Border.BackgroundProperty, PreviewRenderer.ParseBrush("#252525"));
        border.SetValue(Border.BorderBrushProperty, PreviewRenderer.ParseBrush("#4A4A4A"));
        border.SetValue(Border.BorderThicknessProperty, new Thickness(1));
        var listBox = new FrameworkElementFactory(typeof(CompletionListBox));
        listBox.Name = "PART_ListBox";
        listBox.SetValue(Control.BackgroundProperty, PreviewRenderer.ParseBrush("#252525"));
        listBox.SetValue(Control.BorderThicknessProperty, new Thickness(0));
        listBox.SetValue(Control.TemplateProperty, this.CreateCompletionListBoxTemplate());
        var itemTemplate = new DataTemplate(typeof(ICompletionData));
        var content = new FrameworkElementFactory(typeof(ContentPresenter));
        content.SetBinding(ContentPresenter.ContentProperty, new Binding("Content"));
        itemTemplate.VisualTree = content;
        listBox.SetValue(ItemsControl.ItemTemplateProperty, itemTemplate);
        border.AppendChild(listBox);
        template.VisualTree = border;
        return template;
    }

    private ControlTemplate CreateCompletionListBoxTemplate() {
        var template = new ControlTemplate(typeof(CompletionListBox));
        var border = new FrameworkElementFactory(typeof(Border));
        border.SetValue(Border.BackgroundProperty, PreviewRenderer.ParseBrush("#252525"));
        var itemsPresenter = new FrameworkElementFactory(typeof(ItemsPresenter));
        itemsPresenter.Name = "PART_ItemsPresenter";
        border.AppendChild(itemsPresenter);
        template.VisualTree = border;
        return template;
    }

    private void InsertClosingTag() {
        var caretOffset = this.editor.TextArea.Caret.Offset;
        var text = this.editor.Document.Text;
        var openingStart = text.LastIndexOf('<', Math.Max(0, caretOffset - 1));
        if (openingStart < 0) {
            return;
        }

        var openingTag = text[openingStart..caretOffset];
        if (openingTag.StartsWith("</", StringComparison.Ordinal)
            || openingTag.EndsWith("/>", StringComparison.Ordinal)
            || openingTag[..^1].Contains('>')
            || !this.TryGetElementName(openingTag, out var elementName)) {
            return;
        }

        var closingTag = $"</{elementName}>";
        if (text.AsSpan(caretOffset).TrimStart().StartsWith(closingTag, StringComparison.Ordinal)) {
            return;
        }

        this.editor.Document.Insert(caretOffset, closingTag);
        this.editor.TextArea.Caret.Offset = caretOffset;
    }

    private bool TryGetOpenElementName(out string elementName) {
        var caretOffset = this.editor.TextArea.Caret.Offset;
        var text = this.editor.Document.Text;
        var openingStart = text.LastIndexOf('<', Math.Max(0, caretOffset - 1));
        if (openingStart < 0) {
            elementName = string.Empty;
            return false;
        }

        var openingTag = text[openingStart..caretOffset];
        if (openingTag.Contains('>') || openingTag.StartsWith("</", StringComparison.Ordinal)) {
            elementName = string.Empty;
            return false;
        }

        return this.TryGetElementName(openingTag, out elementName);
    }

    private bool TryGetElementName(string tag, out string elementName) {
        var span = tag.AsSpan().TrimStart('<').Trim();
        var end = span.IndexOfAny(' ', '/', '>');
        elementName = end < 0 ? span.ToString() : span[..end].ToString();
        return elementName.Length > 0 && !elementName.StartsWith('/');
    }

    private static IEnumerable<string> GetAttributes(string elementName) {
        var attributes = XamlCompletionController.CommonAttributes.ToList();
        attributes.AddRange(elementName switch {
            "Page" => ["background", "borderBrush", "borderThickness", "padding"],
            "StackPanel" => ["background", "borderBrush", "borderThickness", "padding", "orientation"],
            "TextBlock" => ["text", "fontSize", "fontFamily", "fontWeight", "foreground"],
            "Button" => ["background", "borderBrush", "borderThickness", "padding", "cornerRadius", "text", "command"],
            "Border" => ["background", "borderBrush", "borderThickness", "padding", "cornerRadius"],
            "ToggleSwitch" => ["background", "borderBrush", "borderThickness", "text", "tint", "isOn"],
            "Grid" => ["background", "borderBrush", "borderThickness", "padding", "columns", "rows"],
            "ScrollViewer" => ["background", "borderBrush", "borderThickness", "padding"],
            "Image" or "SvgImage" => ["source", "tint"],
            "IconButton" => ["background", "borderBrush", "borderThickness", "padding", "cornerRadius", "source", "tint", "command"],
            "ListView" => ["background", "borderBrush", "borderThickness", "padding", "itemsSource"],
            "columnDefinition" => ["width"],
            "rowDefinition" => ["height"],
            _ => [],
        });
        return attributes.Distinct(StringComparer.Ordinal);
    }
}

internal sealed class XamlCompletionData : ICompletionData {
    private readonly int? caretOffset;
    private readonly string description;

    public XamlCompletionData(string text, string description, int? caretOffset = null) {
        this.Text = text;
        this.description = description;
        this.caretOffset = caretOffset;
    }

    public ImageSource? Image => null;
    public string Text { get; }
    public object Content => this.CreateContent();
    public object Description => null!;
    public double Priority => 0.0;

    public void Complete(TextArea textArea, ISegment completionSegment, EventArgs insertionRequestEventArgs) {
        textArea.Document.Replace(completionSegment, this.Text);
        textArea.Caret.Offset = completionSegment.Offset + (this.caretOffset ?? this.Text.Length);
    }

    private Grid CreateContent() {
        var grid = new Grid();
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1.0, GridUnitType.Star) });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        var icon = new TextBlock {
            Foreground = PreviewRenderer.ParseBrush("#D5BD7D"),
            Text = "◇",
            Width = 18,
        };
        var name = new TextBlock {
            Text = this.Text,
        };
        var detail = new TextBlock {
            Foreground = PreviewRenderer.ParseBrush("#A8A8A8"),
            Margin = new Thickness(18, 0, 0, 0),
            Text = this.description,
        };
        Grid.SetColumn(name, 1);
        Grid.SetColumn(detail, 2);
        grid.Children.Add(icon);
        grid.Children.Add(name);
        grid.Children.Add(detail);
        return grid;
    }
}