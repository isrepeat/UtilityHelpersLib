using System.Text.Json.Nodes;

namespace XamlPreviewer;

internal static class AlarmPreviewInteraction {
    private const string AlarmsPropertyName = "Alarms";
    private const string AddAlarmButtonId = "addAlarmButton";
    private const string AlarmsListId = "alarms";
    private const float AlarmItemHeight = 200.0f;

    public static bool TryAddAlarm(JsonObject scenario, string elementId) {
        if (elementId != AlarmPreviewInteraction.AddAlarmButtonId
            || scenario[AlarmPreviewInteraction.AlarmsPropertyName] is not JsonArray alarms) {
            return false;
        }
        alarms.Add(new JsonObject {
            ["Time"] = string.Empty,
            ["Repeat"] = string.Empty,
            ["IsEnabled"] = false,
        });
        NativeRuntime.xr_log_info($"Alarm added; total={alarms.Count}.");
        return true;
    }

    public static bool TryRemoveAlarm(JsonObject scenario, PreviewSession session, IntPtr element) {
        if (scenario[AlarmPreviewInteraction.AlarmsPropertyName] is not JsonArray alarms
            || !session.TryGetElementBounds(element, out var itemBounds)
            || !session.TryGetElementBounds(AlarmPreviewInteraction.AlarmsListId, out var listBounds)) {
            return false;
        }
        var index = (int)MathF.Round((itemBounds.Y - listBounds.Y) / AlarmPreviewInteraction.AlarmItemHeight);
        if (index < 0 || index >= alarms.Count) {
            return false;
        }
        alarms.RemoveAt(index);
        NativeRuntime.xr_log_info($"Alarm removed; index={index}, total={alarms.Count}.");
        return true;
    }

}