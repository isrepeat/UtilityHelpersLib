using ICSharpCode.AvalonEdit;
using ICSharpCode.AvalonEdit.Document;
using ICSharpCode.AvalonEdit.Rendering;
using System.Windows;
using System.Windows.Media;

namespace XamlPreviewer;

internal sealed class XmlIndentationGuideRenderer : IBackgroundRenderer {
    private const int IndentationSize = 4;
    private readonly Pen pen;

    public KnownLayer Layer => KnownLayer.Background;

    public XmlIndentationGuideRenderer() {
        this.pen = new Pen(
            new SolidColorBrush(Color.FromArgb(105, 150, 150, 150)),
            1.0) {
            DashStyle = DashStyles.Dot,
        };
        this.pen.Freeze();
    }

    public void Draw(TextView textView, DrawingContext drawingContext) {
        if (!textView.VisualLinesValid || textView.Document is null) {
            return;
        }

        foreach (var visualLine in textView.VisualLines) {
            var line = visualLine.FirstDocumentLine;
            var indentationLength = this.GetIndentationLength(textView.Document, line);
            for (var guideOffset = XmlIndentationGuideRenderer.IndentationSize;
                 guideOffset <= indentationLength;
                 guideOffset += XmlIndentationGuideRenderer.IndentationSize) {
                var position = textView.GetVisualPosition(
                    new TextViewPosition(new TextLocation(line.LineNumber, guideOffset - 1)),
                    VisualYPosition.LineTop);
                drawingContext.DrawLine(
                    this.pen,
                    new Point(position.X, visualLine.VisualTop),
                    new Point(position.X, visualLine.VisualTop + visualLine.Height));
            }
        }
    }

    private int GetIndentationLength(TextDocument document, DocumentLine line) {
        var length = 0;
        while (length < line.Length) {
            var character = document.GetCharAt(line.Offset + length);
            if (character == ' ') {
                ++length;
                continue;
            }
            if (character == '\t') {
                length += XmlIndentationGuideRenderer.IndentationSize;
                continue;
            }
            break;
        }
        return length;
    }
}