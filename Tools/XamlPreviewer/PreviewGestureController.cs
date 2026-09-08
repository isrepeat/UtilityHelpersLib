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
        if (!ScenarioInteraction.HandleTap(scenario, elementId)) {
            return false;
        }
        this.saveChanges(root);
        return true;
    }

    public bool HandlePan(string elementId, int itemIndex) {
        var root = this.getScenarioRoot();
        var scenario = this.getSelectedScenario(root);
        if (!ScenarioInteraction.HandlePan(scenario, elementId, itemIndex)) {
            return false;
        }
        this.saveChanges(root);
        return true;
    }

}