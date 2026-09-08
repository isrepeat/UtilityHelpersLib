using System.Text.Json.Nodes;

namespace XamlPreviewer;

internal static class AlarmPreviewInteraction {
    private const string AlarmsPropertyName = "Alarms";
    private const string AddAlarmButtonId = "addAlarmButton";
    private const string AlarmBlockId = "alarmBlock";

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
        IntPtr listRemovalTransition) {
        if (elementId != AlarmPreviewInteraction.AlarmBlockId
            || scenario[AlarmPreviewInteraction.AlarmsPropertyName] is not JsonArray alarms) {
            return false;
        }
        var index = session.GetListItemIndex(listRemovalTransition);
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