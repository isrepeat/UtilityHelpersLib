using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;

namespace XamlPreviewer;

internal sealed class MarkupEditorController {
    private readonly RichTextBox editor;
    private readonly DispatcherTimer highlightTimer;
    private readonly Stack<EditorState> redoHistory = new();
    private readonly Stack<EditorState> undoHistory = new();
    private EditorState currentState = new(string.Empty, 0, 0);
    private bool isUpdating;
    private int? navigationColumn;

    public event EventHandler? MarkupChanged;

    public string Text => MarkupSyntaxHighlighter.GetText(this.editor);

    public MarkupEditorController(RichTextBox editor) {
        this.editor = editor;
        this.highlightTimer = new DispatcherTimer {
            Interval = TimeSpan.FromMilliseconds(150)
        };
        this.highlightTimer.Tick += this.HighlightTimerTick;
        this.currentState = this.GetEditorState();
    }

    public bool HandleTextChanged() {
        if (this.isUpdating) {
            return false;
        }

        this.RecordTextChange();
        this.highlightTimer.Stop();
        this.highlightTimer.Start();
        this.navigationColumn = null;
        return true;
    }

    public void HandlePreviewKeyDown(KeyEventArgs eventArgs) {
        var key = eventArgs.Key == Key.System ? eventArgs.SystemKey : eventArgs.Key;
        if (Keyboard.Modifiers == ModifierKeys.Control && key == Key.Z) {
            this.Undo();
            eventArgs.Handled = true;
        }
        else if (Keyboard.Modifiers == ModifierKeys.Control && key == Key.Y) {
            this.Redo();
            eventArgs.Handled = true;
        }
        else if (Keyboard.Modifiers == ModifierKeys.Control && key == Key.W) {
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
        else if (Keyboard.Modifiers == ModifierKeys.None && key is Key.Up or Key.Down) {
            this.MoveCaret(key == Key.Up ? -1 : 1);
            eventArgs.Handled = true;
        }
        else if (key is not Key.Left and not Key.Right) {
            this.navigationColumn = null;
        }
    }

    public void SetText(string text) {
        this.highlightTimer.Stop();
        this.isUpdating = true;
        try {
            this.SetEditorText(text, text.Length, text.Length);
        }
        finally {
            this.isUpdating = false;
        }

        this.currentState = this.GetEditorState();
        this.undoHistory.Clear();
        this.redoHistory.Clear();
        this.navigationColumn = null;
    }

    private void InsertNewLine() {
        var text = this.currentState.Text;
        var selection = GetSelection();
        var indentation = MarkupEditorController.GetIndentation(text, selection.Start);
        var newText = text[..selection.Start] + Environment.NewLine + indentation + text[selection.End..];
        this.ReplaceText(newText, selection.Start + Environment.NewLine.Length + indentation.Length);
    }

    private void Undo() {
        if (!this.undoHistory.TryPop(out var state)) {
            return;
        }

        this.redoHistory.Push(this.currentState);
        this.ApplyState(state);
    }

    private void Redo() {
        if (!this.redoHistory.TryPop(out var state)) {
            return;
        }

        this.undoHistory.Push(this.currentState);
        this.ApplyState(state);
    }

    private void SelectWord() {
        var text = this.currentState.Text;
        var selection = this.GetSelection();
        if (text.Length == 0) {
            return;
        }

        var position = Math.Min(selection.Start, text.Length - 1);
        if (!MarkupEditorController.IsWordCharacter(text[position]) && position > 0 && MarkupEditorController.IsWordCharacter(text[position - 1])) {
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

        MarkupSyntaxHighlighter.SetSelection(this.editor, start, end);
        this.currentState = new EditorState(this.currentState.Text, start, end);
    }

    private void DuplicateLine() {
        var text = this.currentState.Text;
        var selection = this.GetSelection();
        var lines = MarkupEditorController.GetLines(text);
        var lineIndex = MarkupEditorController.FindLineIndex(lines, selection.Start);
        if (lineIndex < 0) {
            return;
        }

        var line = lines[lineIndex];
        var lineEnd = MarkupEditorController.GetLineEndWithBreak(lines, lineIndex, text.Length);
        var lineText = text.Substring(line.Start, lineEnd - line.Start);
        var insertion = lineText.EndsWith("\n", StringComparison.Ordinal)
            ? lineText
            : Environment.NewLine + lineText;
        var newText = text.Insert(lineEnd, insertion);
        var column = Math.Min(selection.Start - line.Start, line.Length);
        this.ReplaceText(newText, lineEnd + insertion.Length - lineText.Length + column);
    }

    private void MoveLine(int lineDelta) {
        var text = this.currentState.Text;
        var selection = this.GetSelection();
        var lines = MarkupEditorController.GetLines(text);
        var lineIndex = MarkupEditorController.FindLineIndex(lines, selection.Start);
        var targetLineIndex = lineIndex + lineDelta;
        if (lineIndex < 0 || targetLineIndex < 0 || targetLineIndex >= lines.Count) {
            return;
        }

        var currentLine = lines[lineIndex];
        var targetLine = lines[targetLineIndex];
        var currentEnd = MarkupEditorController.GetLineEndWithBreak(lines, lineIndex, text.Length);
        var targetEnd = MarkupEditorController.GetLineEndWithBreak(lines, targetLineIndex, text.Length);
        var regionStart = Math.Min(currentLine.Start, targetLine.Start);
        var regionEnd = Math.Max(currentEnd, targetEnd);
        var currentText = text.Substring(currentLine.Start, currentEnd - currentLine.Start);
        var targetText = text.Substring(targetLine.Start, targetEnd - targetLine.Start);
        var newText = text[..regionStart] + (lineDelta < 0 ? currentText + targetText : targetText + currentText) + text[regionEnd..];
        var column = Math.Min(selection.Start - currentLine.Start, currentLine.Length);
        var newLineStart = lineDelta < 0 ? targetLine.Start : currentLine.Start + targetText.Length;
        this.ReplaceText(newText, newLineStart + column);
    }

    private void MoveCaret(int lineDelta) {
        var text = this.currentState.Text;
        var selection = GetSelection();
        if (selection.Start != selection.End) {
            return;
        }

        var lines = MarkupEditorController.GetLines(text);
        var lineIndex = MarkupEditorController.FindLineIndex(lines, selection.Start);
        if (lineIndex < 0) {
            return;
        }

        var targetLineIndex = lineIndex + lineDelta;
        if (targetLineIndex < 0 || targetLineIndex >= lines.Count) {
            return;
        }

        var currentLine = lines[lineIndex];
        var currentColumn = selection.Start - currentLine.Start;
        this.navigationColumn ??= currentColumn;
        var targetLine = lines[targetLineIndex];
        var targetColumn = Math.Min(this.navigationColumn.Value, targetLine.Length);
        targetColumn = Math.Max(targetColumn, targetLine.IndentationLength);
        this.ReplaceSelection(targetLine.Start + targetColumn);
    }

    private void ReplaceText(string text, int caretOffset) {
        this.highlightTimer.Stop();
        this.undoHistory.Push(this.currentState);
        this.redoHistory.Clear();
        this.isUpdating = true;
        try {
            this.SetEditorText(text, caretOffset, caretOffset);
        }
        finally {
            this.isUpdating = false;
        }

        this.currentState = this.GetEditorState();
        this.navigationColumn = null;
        this.MarkupChanged?.Invoke(this, EventArgs.Empty);
    }

    private void ReplaceSelection(int offset) {
        MarkupSyntaxHighlighter.SetSelection(this.editor, offset, offset);
        this.currentState = new EditorState(this.currentState.Text, offset, offset);
    }

    private void HighlightTimerTick(object? sender, EventArgs eventArgs) {
        this.highlightTimer.Stop();
        this.isUpdating = true;
        try {
            MarkupSyntaxHighlighter.Highlight(this.editor);
        }
        finally {
            this.isUpdating = false;
        }
    }

    private void RecordTextChange() {
        var state = this.GetEditorState();
        if (state.Text == this.currentState.Text) {
            return;
        }

        this.undoHistory.Push(this.currentState);
        this.redoHistory.Clear();
        this.currentState = state;
    }

    private void ApplyState(EditorState state) {
        this.highlightTimer.Stop();
        this.isUpdating = true;
        try {
            this.SetEditorText(state.Text, state.SelectionStart, state.SelectionEnd);
        }
        finally {
            this.isUpdating = false;
        }

        this.currentState = state;
        this.navigationColumn = null;
        this.MarkupChanged?.Invoke(this, EventArgs.Empty);
    }

    private EditorState GetEditorState() {
        var selection = this.GetSelection();
        return new EditorState(this.Text, selection.Start, selection.End);
    }

    private void SetEditorText(string text, int selectionStart, int selectionEnd) {
        MarkupSyntaxHighlighter.SetText(this.editor, text, selectionStart, selectionEnd);
    }

    private (int Start, int End) GetSelection() {
        var start = this.editor.Document.ContentStart;
        return (
            new System.Windows.Documents.TextRange(start, this.editor.Selection.Start).Text.Length,
            new System.Windows.Documents.TextRange(start, this.editor.Selection.End).Text.Length);
    }

    private static string GetIndentation(string text, int offset) {
        var lines = MarkupEditorController.GetLines(text);
        var line = lines.FirstOrDefault(candidate => offset >= candidate.Start && offset <= candidate.End);
        return line is null ? string.Empty : line.Text[..line.IndentationLength];
    }

    private static int FindLineIndex(IReadOnlyList<LineInfo> lines, int offset) {
        for (var index = 0; index < lines.Count; ++index) {
            var line = lines[index];
            if (offset >= line.Start && offset <= line.End) {
                return index;
            }
        }

        return -1;
    }

    private static int GetLineEndWithBreak(IReadOnlyList<LineInfo> lines, int lineIndex, int textLength) {
        return lineIndex + 1 < lines.Count ? lines[lineIndex + 1].Start : textLength;
    }

    private static bool IsWordCharacter(char character) {
        return char.IsLetterOrDigit(character) || character is '_' or ':' or '-' or '.';
    }

    private static List<LineInfo> GetLines(string text) {
        var lines = new List<LineInfo>();
        var start = 0;
        while (start <= text.Length) {
            var end = text.IndexOf('\n', start);
            if (end < 0) {
                end = text.Length;
            }

            var length = end - start;
            if (length > 0 && text[start + length - 1] == '\r') {
                --length;
            }

            var lineText = text.Substring(start, length);
            lines.Add(new LineInfo(start, lineText));
            if (end == text.Length) {
                break;
            }

            start = end + 1;
        }

        return lines;
    }

    private sealed class LineInfo {
        public int End => this.Start + this.Length;
        public int IndentationLength { get; }
        public int Length => this.Text.Length;
        public int Start { get; }
        public string Text { get; }

        public LineInfo(int start, string text) {
            this.Start = start;
            this.Text = text;
            this.IndentationLength = text.TakeWhile(character => character is ' ' or '\t').Count();
        }
    }

    private sealed class EditorState {
        public int SelectionEnd { get; }
        public int SelectionStart { get; }
        public string Text { get; }

        public EditorState(string text, int selectionStart, int selectionEnd) {
            this.Text = text;
            this.SelectionStart = selectionStart;
            this.SelectionEnd = selectionEnd;
        }
    }
}