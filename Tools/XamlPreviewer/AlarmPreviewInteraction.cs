using System.Text.Json.Nodes;

namespace XamlPreviewer;

internal static class AlarmPreviewInteraction {
    private const string AlarmsPropertyName = "Alarms";
    private const string AlarmsListId = "alarms";
    private const string AddAlarmButtonId = "addAlarmButton";
    private const string AlarmBlockId = "alarmBlock";
    private const float AlarmItemBottomMargin = 16.0f;

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

    public static bool TryRemoveAlarm(
        JsonObject scenario,
        PreviewSession session,
        string elementId,
        NativeRect alarmBounds) {
        if (elementId != AlarmPreviewInteraction.AlarmBlockId
            || scenario[AlarmPreviewInteraction.AlarmsPropertyName] is not JsonArray alarms
            || !session.TryGetElementBounds(AlarmPreviewInteraction.AlarmsListId, out var alarmsBounds)) {
            return false;
        }
        var itemHeight = alarmBounds.Height + AlarmPreviewInteraction.AlarmItemBottomMargin;
        var index = (int)Math.Round((alarmBounds.Y - alarmsBounds.Y) / itemHeight);
        if (index < 0 || index >= alarms.Count) {
            NativeRuntime.xr_log_info($"Alarm removal rejected; calculatedIndex={index}, total={alarms.Count}.");
            return false;
        }
        var countBeforeRemoval = alarms.Count;
        alarms.RemoveAt(index);
        NativeRuntime.xr_log_info($"Alarm removed; index={index}, total={countBeforeRemoval - 1}.");
        return true;
    }
}