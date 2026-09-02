using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace XamlPreviewer;

internal sealed class AnglePreviewRenderer : IDisposable {
    public const int Width = 720;
    public const int Height = 1280;

    private const string FontPath = @"C:\WORK\Android\Projects\MobileClock\app\src\main\assets\Roboto-Regular.ttf";

    private IntPtr surface;

    public AnglePreviewRenderer(string markupDirectory) {
        this.surface = NativeRuntime.xr_create_angle_surface(
            AnglePreviewRenderer.Width,
            AnglePreviewRenderer.Height,
            AnglePreviewRenderer.FontPath,
            markupDirectory);
        NativeRuntime.Ensure(this.surface != IntPtr.Zero);
    }

    public BitmapSource Render(IntPtr root) {
        const int bytesPerPixel = 4;
        NativeRuntime.Ensure(NativeRuntime.xr_layout(root, AnglePreviewRenderer.Width, AnglePreviewRenderer.Height) != 0);
        var stride = AnglePreviewRenderer.Width * bytesPerPixel;
        var pixels = new byte[stride * AnglePreviewRenderer.Height];
        NativeRuntime.Ensure(NativeRuntime.xr_render_angle_surface(
            this.surface,
            root,
            pixels,
            stride,
            pixels.Length) != 0);
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
        return bitmap;
    }

    public void Dispose() {
        if (this.surface == IntPtr.Zero) {
            return;
        }
        NativeRuntime.xr_destroy_angle_surface(this.surface);
        this.surface = IntPtr.Zero;
    }
}