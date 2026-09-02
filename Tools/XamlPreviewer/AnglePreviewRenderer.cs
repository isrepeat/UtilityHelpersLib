using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace XamlPreviewer;

internal static class AnglePreviewRenderer {
    // Размер соответствует базовой области предпросмотра мобильной разметки.
    private const int Width = 720;
    private const int Height = 1280;
    // Нативный rasterizer строит atlas из того же Roboto, что используется приложением Android.
    private const string FontPath = @"C:\WORK\Android\Projects\MobileClock\app\src\main\assets\Roboto-Regular.ttf";

    public static FrameworkElement Render(IntPtr root) {
        const int bytesPerPixel = 4;
        NativeRuntime.Ensure(NativeRuntime.xr_layout(
            root,
            AnglePreviewRenderer.Width,
            AnglePreviewRenderer.Height) != 0);
        var stride = AnglePreviewRenderer.Width * bytesPerPixel;
        var pixels = new byte[stride * AnglePreviewRenderer.Height];
        NativeRuntime.Ensure(NativeRuntime.xr_render_angle(
            root,
            AnglePreviewRenderer.FontPath,
            AnglePreviewRenderer.Width,
            AnglePreviewRenderer.Height,
            pixels,
            stride,
            pixels.Length) != 0);

        // Native bridge возвращает BGRA-буфер. Freeze позволяет безопасно отдать
        // неизменяемый bitmap элементу WPF без удержания native-ресурсов.
        var bitmap = BitmapSource.Create(
            AnglePreviewRenderer.Width,
            AnglePreviewRenderer.Height,
            96,
            96,
            PixelFormats.Bgra32,
            null,
            pixels,
            stride);
        bitmap.Freeze();
        return new Image {
            Width = AnglePreviewRenderer.Width,
            Height = AnglePreviewRenderer.Height,
            Source = bitmap,
            Stretch = Stretch.Fill
        };
    }
}