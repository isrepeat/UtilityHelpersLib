using System.Text.Json.Nodes;

namespace XamlPreviewer;

internal sealed class PreviewGestureController {
    private readonly Func<JsonObject> getScenarioRoot;
    private readonly Func<JsonObject, JsonObject> getSelectedScenario;
    private readonly Action<JsonObject> saveChanges;

    public PreviewGestureController(
        Func<JsonObject> getScenarioRoot,
        Func<JsonObject, JsonObject> getSelectedScenario,
        Action<JsonObject> saveChanges) {
        this.getScenarioRoot = getScenarioRoot;
        this.getSelectedScenario = getSelectedScenario;
        this.saveChanges = saveChanges;
    }

    public bool HandleTap(string elementId) {
        var root = this.getScenarioRoot();
        var scenario = this.getSelectedScenario(root);
        if (!AlarmPreviewInteraction.TryAddAlarm(scenario, elementId)) {
            return false;
        }
        this.saveChanges(root);
        return true;
    }

    public bool HandleSwipe(PreviewSession session, IntPtr element) {
        var root = this.getScenarioRoot();
        var scenario = this.getSelectedScenario(root);
        if (!AlarmPreviewInteraction.TryRemoveAlarm(scenario, session, element)) {
            return false;
        }
        this.saveChanges(root);
        return true;
    }

}