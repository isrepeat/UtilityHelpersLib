using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Media;

namespace XamlPreviewer;

internal static class MarkupSyntaxHighlighter {
    private static readonly Brush AttributeBrush = MarkupSyntaxHighlighter.CreateBrush("#8BE9FD");
    private static readonly Brush BindingBrush = MarkupSyntaxHighlighter.CreateBrush("#C792EA");
    private static readonly Brush StringBrush = MarkupSyntaxHighlighter.CreateBrush("#F0D78C");
    private static readonly Brush TagBrush = MarkupSyntaxHighlighter.CreateBrush("#7DCFFF");
    private static readonly Brush TextBrush = MarkupSyntaxHighlighter.CreateBrush("#E6E6E6");
    private static readonly Regex AttributeRegex = new(@"[\w:.-]+(?=\s*=)");
    private static readonly Regex BindingRegex = new(@"\{Binding\b[^}]*\}");
    private static readonly Regex StringRegex = new("\"[^\"]*\"");
    private static readonly Regex TagRegex = new(@"</?[\w:.-]+");

    public static string GetText(RichTextBox editor) {
        var text = new TextRange(
            editor.Document.ContentStart,
            editor.Document.ContentEnd).Text;
        return text.EndsWith("\r\n", StringComparison.Ordinal)
            ? text[..^2]
            : text;
    }

    public static void Highlight(RichTextBox editor) {
        var text = MarkupSyntaxHighlighter.GetText(editor);
        var selectionStart = MarkupSyntaxHighlighter.GetTextOffset(editor.Document.ContentStart, editor.Selection.Start);
        var selectionEnd = MarkupSyntaxHighlighter.GetTextOffset(editor.Document.ContentStart, editor.Selection.End);
        MarkupSyntaxHighlighter.SetText(editor, text, selectionStart, selectionEnd);
    }

    public static void SetText(RichTextBox editor, string text) {
        MarkupSyntaxHighlighter.SetText(editor, text, text.Length, text.Length);
    }

    public static void SetText(RichTextBox editor, string text, int selectionStart, int selectionEnd) {
        var paragraph = new Paragraph {
            Margin = new Thickness(0)
        };
        var spans = MarkupSyntaxHighlighter.CreateSyntaxSpans(text);
        var offset = 0;
        while (offset < text.Length) {
            var span = spans.FirstOrDefault(candidate => candidate.Start == offset);
            if (span is not null) {
                paragraph.Inlines.Add(new Run(text.Substring(span.Start, span.Length)) {
                    Foreground = span.Brush
                });
                offset += span.Length;
            }
            else {
                var next = spans.Where(candidate => candidate.Start > offset)
                    .Select(candidate => candidate.Start)
                    .DefaultIfEmpty(text.Length)
                    .Min();
                paragraph.Inlines.Add(new Run(text.Substring(offset, next - offset)) {
                    Foreground = TextBrush
                });
                offset = next;
            }
        }

        editor.Document.Blocks.Clear();
        editor.Document.Blocks.Add(paragraph);
        MarkupSyntaxHighlighter.SetSelection(editor, selectionStart, selectionEnd);
    }

    public static void SetSelection(RichTextBox editor, int selectionStart, int selectionEnd) {
        editor.Selection.Select(
            MarkupSyntaxHighlighter.GetTextPointerAtOffset(editor, selectionStart),
            MarkupSyntaxHighlighter.GetTextPointerAtOffset(editor, selectionEnd));
    }

    private static void ApplySyntaxBrush(
        Brush?[] brushes,
        Regex regex,
        string text,
        Brush brush) {
        foreach (Match match in regex.Matches(text)) {
            for (var index = match.Index; index < match.Index + match.Length; ++index) {
                brushes[index] = brush;
            }
        }
    }

    private static List<SyntaxSpan> CreateSyntaxSpans(string text) {
        var brushes = new Brush?[text.Length];
        MarkupSyntaxHighlighter.ApplySyntaxBrush(brushes, TagRegex, text, TagBrush);
        MarkupSyntaxHighlighter.ApplySyntaxBrush(brushes, AttributeRegex, text, AttributeBrush);
        MarkupSyntaxHighlighter.ApplySyntaxBrush(brushes, StringRegex, text, StringBrush);
        MarkupSyntaxHighlighter.ApplySyntaxBrush(brushes, BindingRegex, text, BindingBrush);

        var spans = new List<SyntaxSpan>();
        var start = 0;
        while (start < brushes.Length) {
            var brush = brushes[start];
            if (brush is null) {
                ++start;
                continue;
            }

            var end = start + 1;
            while (end < brushes.Length && ReferenceEquals(brushes[end], brush)) {
                ++end;
            }

            spans.Add(new SyntaxSpan(start, end - start, brush));
            start = end;
        }

        return spans;
    }

    private static Brush CreateBrush(string value) {
        var brush = PreviewRenderer.ParseBrush(value);
        brush.Freeze();
        return brush;
    }

    private static int GetTextOffset(TextPointer start, TextPointer position) {
        return new TextRange(start, position).Text.Length;
    }

    private static TextPointer GetTextPointerAtOffset(RichTextBox editor, int offset) {
        var pointer = editor.Document.ContentStart;
        while (pointer.CompareTo(editor.Document.ContentEnd) < 0) {
            if (pointer.GetPointerContext(LogicalDirection.Forward) == TextPointerContext.Text) {
                var text = pointer.GetTextInRun(LogicalDirection.Forward);
                if (offset <= text.Length) {
                    return pointer.GetPositionAtOffset(offset) ?? editor.Document.ContentEnd;
                }

                offset -= text.Length;
                pointer = pointer.GetPositionAtOffset(text.Length) ?? editor.Document.ContentEnd;
            }
            else {
                pointer = pointer.GetNextContextPosition(LogicalDirection.Forward)
                    ?? editor.Document.ContentEnd;
            }
        }

        return editor.Document.ContentEnd;
    }

    private sealed class SyntaxSpan {
        public Brush Brush { get; }
        public int Length { get; }
        public int Start { get; }

        public SyntaxSpan(int start, int length, Brush brush) {
            this.Start = start;
            this.Length = length;
            this.Brush = brush;
        }
    }
}