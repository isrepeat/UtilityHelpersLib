using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;

namespace XamlPreviewer;

internal sealed class PreviewSession : IDisposable {
    private readonly AnglePreviewRenderer renderer;
    private readonly Image image;
    private IntPtr root;
    private IntPtr animations;
    private IntPtr capturedElement;
    private bool isDisposed;

    public PreviewSession(IntPtr root, string markupDirectory, int width, int height) {
        this.root = root;
        this.renderer = new AnglePreviewRenderer(markupDirectory, width, height);
        this.animations = NativeRuntime.xr_create_animation_controller();
        NativeRuntime.Ensure(this.animations != IntPtr.Zero);
        this.image = new Image {
            Width = this.renderer.Width,
            Height = this.renderer.Height,
            Stretch = Stretch.Fill
        };
        this.image.MouseLeftButtonDown += this.ImageMouseLeftButtonDown;
        this.image.MouseLeftButtonUp += this.ImageMouseLeftButtonUp;
        this.image.MouseMove += this.ImageMouseMove;
        this.image.MouseLeave += this.ImageMouseLeave;
        this.Render();
    }

    public FrameworkElement Surface => this.image;

    public ImageSource? Snapshot => this.image.Source;

    public event EventHandler? AnimationStarted;
    public event EventHandler<string>? Tapped;

    public bool Update() {
        this.ThrowIfDisposed();
        var isAnimating = NativeRuntime.xr_update_animations(this.animations);
        NativeRuntime.Ensure(isAnimating >= 0);
        if (isAnimating == 0) {
            return false;
        }
        this.Render();
        return true;
    }

    public void Dispose() {
        if (this.isDisposed) {
            return;
        }
        this.isDisposed = true;
        this.image.MouseLeftButtonDown -= this.ImageMouseLeftButtonDown;
        this.image.MouseLeftButtonUp -= this.ImageMouseLeftButtonUp;
        this.image.MouseMove -= this.ImageMouseMove;
        this.image.MouseLeave -= this.ImageMouseLeave;
        if (this.animations != IntPtr.Zero) {
            NativeRuntime.xr_destroy_animation_controller(this.animations);
            this.animations = IntPtr.Zero;
        }
        if (this.root != IntPtr.Zero) {
            NativeRuntime.xr_destroy_element(this.root);
            this.root = IntPtr.Zero;
        }
        this.renderer.Dispose();
    }

    private void ImageMouseLeftButtonDown(object sender, MouseButtonEventArgs eventArgs) {
        var point = eventArgs.GetPosition(this.image);
        this.capturedElement = NativeRuntime.xr_hit_test(
            this.root,
            this.ScaleX(point.X),
            this.ScaleY(point.Y));
        if (this.capturedElement != IntPtr.Zero) {
            this.image.CaptureMouse();
            NativeRuntime.Ensure(NativeRuntime.xr_handle_pointer_down(this.capturedElement, this.animations) != 0);
            this.Render();
            this.AnimationStarted?.Invoke(this, EventArgs.Empty);
            eventArgs.Handled = true;
        }
    }

    private void ImageMouseLeftButtonUp(object sender, MouseButtonEventArgs eventArgs) {
        if (this.capturedElement == IntPtr.Zero) {
            return;
        }
        var elementId = NativeRuntime.GetElementId(this.capturedElement);
        NativeRuntime.Ensure(NativeRuntime.xr_handle_pointer_up(this.capturedElement, this.animations) != 0);
        this.capturedElement = IntPtr.Zero;
        this.image.ReleaseMouseCapture();
        this.Render();
        this.AnimationStarted?.Invoke(this, EventArgs.Empty);
        this.Tapped?.Invoke(this, elementId);
        eventArgs.Handled = true;
    }

    private void ImageMouseMove(object sender, MouseEventArgs eventArgs) {
        if (this.capturedElement != IntPtr.Zero) {
            return;
        }
        var point = eventArgs.GetPosition(this.image);
        var element = NativeRuntime.xr_hit_test(
            this.root,
            this.ScaleX(point.X),
            this.ScaleY(point.Y));
        this.image.Cursor = element == IntPtr.Zero ? null : Cursors.Hand;
    }

    private void ImageMouseLeave(object sender, MouseEventArgs eventArgs) {
        this.image.Cursor = null;
    }

    private void Render() {
        this.image.Source = this.renderer.Render(this.root);
    }

    private float ScaleX(double value) {
        return (float)(value / this.image.ActualWidth * this.renderer.Width);
    }

    private float ScaleY(double value) {
        return (float)(value / this.image.ActualHeight * this.renderer.Height);
    }

    private void ThrowIfDisposed() {
        ObjectDisposedException.ThrowIf(this.isDisposed, this);
    }
}