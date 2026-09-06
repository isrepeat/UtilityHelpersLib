using ICSharpCode.AvalonEdit;
using ICSharpCode.AvalonEdit.Folding;
using System.Windows.Input;
using System.Windows.Media;

namespace XamlPreviewer;

internal sealed class MarkupEditorController {
    private readonly TextEditor editor;
    private readonly FoldingManager foldingManager;
    private readonly XmlFoldingStrategy foldingStrategy = new();
    private readonly XmlIndentationGuideRenderer indentationGuideRenderer = new();
    private bool isUpdating;

    public string Text => this.editor.Text;

    public MarkupEditorController(TextEditor editor) {
        this.editor = editor;
        this.editor.Options.ConvertTabsToSpaces = false;
        this.editor.Options.IndentationSize = 4;
        this.editor.Options.EnableTextDragDrop = true;
        this.foldingManager = FoldingManager.Install(this.editor.TextArea);
        this.ConfigureFoldingMargin();
        this.editor.TextArea.TextView.BackgroundRenderers.Add(this.indentationGuideRenderer);
        this.editor.Document.Changed += this.DocumentChanged;
        this.UpdateFoldings();
    }

    public bool HandleTextChanged() {
        return !this.isUpdating;
    }

    public void Dispose() {
        this.editor.Document.Changed -= this.DocumentChanged;
        this.editor.TextArea.TextView.BackgroundRenderers.Remove(this.indentationGuideRenderer);
        FoldingManager.Uninstall(this.foldingManager);
    }

    public void HandlePreviewKeyDown(KeyEventArgs eventArgs) {
        var key = eventArgs.Key == Key.System ? eventArgs.SystemKey : eventArgs.Key;
        if (Keyboard.Modifiers == ModifierKeys.Control && key == Key.W) {
            this.SelectWord();
            eventArgs.Handled = true;
        }
        else if (Keyboard.Modifiers == ModifierKeys.Control && key == Key.D) {
            this.DuplicateLine();
            eventArgs.Handled = true;
        }
        else if (Keyboard.Modifiers == ModifierKeys.Alt && key is Key.Up or Key.Down) {
            this.MoveLine(key == Key.Up ? -1 : 1);
            eventArgs.Handled = true;
        }
        else if (key == Key.Enter) {
            this.InsertNewLine();
            eventArgs.Handled = true;
        }
    }

    public void SetText(string text) {
        this.isUpdating = true;
        try {
            this.editor.Text = text;
            this.editor.CaretOffset = text.Length;
            this.editor.SelectionStart = text.Length;
            this.editor.SelectionLength = 0;
            this.editor.Document.UndoStack.ClearAll();
        }
        finally {
            this.isUpdating = false;
        }
    }

    private void InsertNewLine() {
        var indentation = this.GetIndentation(this.editor.CaretOffset);
        var selectionStart = this.editor.SelectionStart;
        this.editor.Document.Replace(
            selectionStart,
            this.editor.SelectionLength,
            Environment.NewLine + indentation);
        this.editor.CaretOffset = selectionStart + Environment.NewLine.Length + indentation.Length;
        this.editor.SelectionLength = 0;
    }

    private void DocumentChanged(
        object? sender,
        ICSharpCode.AvalonEdit.Document.DocumentChangeEventArgs eventArgs) {
        this.UpdateFoldings();
    }

    private void ConfigureFoldingMargin() {
        var foldingMargin = this.editor.TextArea.LeftMargins.OfType<FoldingMargin>().FirstOrDefault();
        if (foldingMargin is null) {
            return;
        }
        foldingMargin.FoldingMarkerBackgroundBrush = MarkupEditorController.CreateBrush("#FF25282C");
        foldingMargin.FoldingMarkerBrush = MarkupEditorController.CreateBrush("#FF9FA7AE");
        foldingMargin.SelectedFoldingMarkerBackgroundBrush = MarkupEditorController.CreateBrush("#FF3A4046");
        foldingMargin.SelectedFoldingMarkerBrush = MarkupEditorController.CreateBrush("#FFF2F4F5");
    }

    private void UpdateFoldings() {
        this.foldingManager.UpdateFoldings(this.foldingStrategy.CreateNewFoldings(this.editor.Document), -1);
    }

    private void SelectWord() {
        var text = this.editor.Text;
        if (text.Length == 0) {
            return;
        }

        var position = Math.Min(this.editor.CaretOffset, text.Length - 1);
        if (!MarkupEditorController.IsWordCharacter(text[position])
            && position > 0
            && MarkupEditorController.IsWordCharacter(text[position - 1])) {
            --position;
        }
        if (!MarkupEditorController.IsWordCharacter(text[position])) {
            return;
        }

        var start = position;
        while (start > 0 && MarkupEditorController.IsWordCharacter(text[start - 1])) {
            --start;
        }
        var end = position + 1;
        while (end < text.Length && MarkupEditorController.IsWordCharacter(text[end])) {
            ++end;
        }
        this.editor.Select(start, end - start);
    }

    private void DuplicateLine() {
        var line = this.editor.Document.GetLineByOffset(this.editor.CaretOffset);
        var lineEnd = line.Offset + line.TotalLength;
        var lineText = this.editor.Document.GetText(line.Offset, line.TotalLength);
        if (!lineText.EndsWith("\n", StringComparison.Ordinal)) {
            lineText = Environment.NewLine + lineText;
        }
        this.editor.Document.Insert(lineEnd, lineText);
        this.editor.CaretOffset = lineEnd + lineText.Length;
        this.editor.SelectionLength = 0;
    }

    private void MoveLine(int lineDelta) {
        if (this.TryMoveSelectedLines(lineDelta)) {
            return;
        }

        var currentLine = this.editor.Document.GetLineByOffset(this.editor.CaretOffset);
        var targetLineNumber = currentLine.LineNumber + lineDelta;
        if (targetLineNumber < 1 || targetLineNumber > this.editor.Document.LineCount) {
            return;
        }

        var targetLine = this.editor.Document.GetLineByNumber(targetLineNumber);
        var regionStart = Math.Min(currentLine.Offset, targetLine.Offset);
        var regionEnd = Math.Max(
            currentLine.Offset + currentLine.TotalLength,
            targetLine.Offset + targetLine.TotalLength);
        var currentText = this.editor.Document.GetText(currentLine.Offset, currentLine.TotalLength);
        var targetText = this.editor.Document.GetText(targetLine.Offset, targetLine.TotalLength);
        var column = Math.Min(this.editor.CaretOffset - currentLine.Offset, currentLine.Length);
        this.editor.Document.Replace(
            regionStart,
            regionEnd - regionStart,
            lineDelta < 0 ? currentText + targetText : targetText + currentText);

        var newOffset = lineDelta < 0
            ? targetLine.Offset + column
            : currentLine.Offset + targetText.Length + column;
        this.editor.CaretOffset = newOffset;
        this.editor.SelectionLength = 0;
    }

    private bool TryMoveSelectedLines(int lineDelta) {
        if (this.editor.SelectionLength == 0) {
            return false;
        }

        var document = this.editor.Document;
        var selectionStart = this.editor.SelectionStart;
        var selectionEnd = selectionStart + this.editor.SelectionLength;
        var firstLine = document.GetLineByOffset(selectionStart);
        var lastOffset = selectionEnd;
        if (selectionEnd > selectionStart
            && selectionEnd == document.GetLineByOffset(selectionEnd).Offset) {
            --lastOffset;
        }
        var lastLine = document.GetLineByOffset(lastOffset);
        if (firstLine.LineNumber == lastLine.LineNumber) {
            return false;
        }

        var targetLineNumber = lineDelta < 0
            ? firstLine.LineNumber - 1
            : lastLine.LineNumber + 1;
        if (targetLineNumber < 1 || targetLineNumber > document.LineCount) {
            return true;
        }

        var targetLine = document.GetLineByNumber(targetLineNumber);
        var blockStart = firstLine.Offset;
        var blockEnd = lastLine.Offset + lastLine.TotalLength;
        var blockText = document.GetText(blockStart, blockEnd - blockStart);
        var targetText = document.GetText(targetLine.Offset, targetLine.TotalLength);
        var regionStart = Math.Min(blockStart, targetLine.Offset);
        var regionEnd = Math.Max(blockEnd, targetLine.Offset + targetLine.TotalLength);
        document.Replace(
            regionStart,
            regionEnd - regionStart,
            lineDelta < 0 ? blockText + targetText : targetText + blockText);

        var newSelectionStart = lineDelta < 0
            ? regionStart
            : blockStart + targetText.Length;
        this.editor.Select(newSelectionStart, blockText.Length);
        return true;
    }

    private string GetIndentation(int offset) {
        var line = this.editor.Document.GetLineByOffset(offset);
        var lineText = this.editor.Document.GetText(line.Offset, line.Length);
        return new string(lineText.TakeWhile(character => character is ' ' or '\t').ToArray());
    }

    private static bool IsWordCharacter(char character) {
        return char.IsLetterOrDigit(character) || character is '_' or ':' or '-' or '.';
    }

    private static SolidColorBrush CreateBrush(string value) {
        var brush = new SolidColorBrush((Color)ColorConverter.ConvertFromString(value));
        brush.Freeze();
        return brush;
    }
}