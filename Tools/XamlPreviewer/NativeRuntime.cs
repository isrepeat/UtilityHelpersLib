using System.Runtime.InteropServices;
using System.Text;

namespace XamlPreviewer;

internal enum NativeCommandType {
    BeginClip,
    EndClip,
    Outline,
    RoundedRect,
    RoundedRectOutline,
    Text,
    Image
}

[StructLayout(LayoutKind.Sequential)]
internal struct NativeRect {
    public float X;
    public float Y;
    public float Width;
    public float Height;
}

[StructLayout(LayoutKind.Sequential)]
internal struct NativeColor {
    public float Red;
    public float Green;
    public float Blue;
    public float Alpha;
}

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeCommand {
    public int Type;
    public NativeRect Bounds;
    public NativeColor Color;
    public float Value;
    public fixed byte Text[512];
    public fixed byte Auxiliary[128];

    public string GetText() {
        fixed (byte* value = this.Text) {
            return NativeCommand.ReadUtf8(value, 512);
        }
    }

    public string GetAuxiliary() {
        fixed (byte* value = this.Auxiliary) {
            return NativeCommand.ReadUtf8(value, 128);
        }
    }

    public float GetAuxiliaryFloat() {
        fixed (byte* value = this.Auxiliary) {
            return *(float*)value;
        }
    }

    private static string ReadUtf8(byte* value, int capacity) {
        var length = 0;
        while (length < capacity && value[length] != 0) {
            ++length;
        }

        return Encoding.UTF8.GetString(value, length);
    }
}

internal static class NativeRuntime {
    private const string Library = "XamlRuntime.NativeBridge.dll";

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, EntryPoint = "xr_create_element")]
    public static extern IntPtr xr_create_element([MarshalAs(UnmanagedType.LPUTF8Str)] string type);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, EntryPoint = "xr_destroy_element")]
    public static extern void xr_destroy_element(IntPtr element);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, EntryPoint = "xr_add_child")]
    public static extern int xr_add_child(IntPtr parent, IntPtr child);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, EntryPoint = "xr_set_attribute")]
    public static extern int xr_set_attribute(
        IntPtr element,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string name,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, EntryPoint = "xr_layout")]
    public static extern int xr_layout(IntPtr root, float width, float height);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, EntryPoint = "xr_hit_test")]
    public static extern IntPtr xr_hit_test(IntPtr root, float x, float y);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, EntryPoint = "xr_element_id")]
    private static extern IntPtr xr_element_id(IntPtr element);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, EntryPoint = "xr_handle_tap")]
    public static extern int xr_handle_tap(IntPtr element, IntPtr animations);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, EntryPoint = "xr_create_animation_controller")]
    public static extern IntPtr xr_create_animation_controller();

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, EntryPoint = "xr_destroy_animation_controller")]
    public static extern void xr_destroy_animation_controller(IntPtr animations);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, EntryPoint = "xr_update_animations")]
    public static extern int xr_update_animations(IntPtr animations);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, EntryPoint = "xr_create_angle_surface")]
    public static extern IntPtr xr_create_angle_surface(
        int width,
        int height,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string fontPath,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string resourceRoot);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, EntryPoint = "xr_destroy_angle_surface")]
    public static extern void xr_destroy_angle_surface(IntPtr surface);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, EntryPoint = "xr_render_angle_surface")]
    public static extern int xr_render_angle_surface(
        IntPtr surface,
        IntPtr root,
        [Out] byte[] pixels,
        int stride,
        int capacity);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, EntryPoint = "xr_render")]
    public static extern int xr_render(IntPtr root, [Out] NativeCommand[]? commands, int capacity);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, EntryPoint = "xr_render_angle")]
    public static extern int xr_render_angle(
        IntPtr root,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string fontPath,
        int width,
        int height,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string resourceRoot,
        [Out] byte[] pixels,
        int stride,
        int capacity);

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl, EntryPoint = "xr_last_error")]
    private static extern IntPtr xr_last_error();

    public static void Ensure(bool result) {
        if (!result) {
            throw new InvalidOperationException(NativeRuntime.GetLastError());
        }
    }

    public static string GetLastError() {
        return Marshal.PtrToStringUTF8(NativeRuntime.xr_last_error()) ?? "Unknown XamlRuntime error.";
    }

    public static string GetElementId(IntPtr element) {
        return Marshal.PtrToStringUTF8(NativeRuntime.xr_element_id(element)) ?? string.Empty;
    }
}