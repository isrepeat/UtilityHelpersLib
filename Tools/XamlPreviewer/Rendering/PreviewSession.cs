using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;

namespace XamlPreviewer;

internal sealed class PreviewSession : IDisposable {
    private readonly AnglePreviewRenderer renderer;
    private readonly Image image;
    private readonly Border inspectionOutline;
    private readonly Grid surface;
    private IntPtr root;
    private IntPtr animations;
    private IntPtr capturedElement;
    private bool isElementInspectionEnabled;
    private bool isDisposed;
    private IReadOnlyDictionary<IntPtr, (int Line, int Column)> sourceLocations =
        new Dictionary<IntPtr, (int Line, int Column)>();

    public PreviewSession(IntPtr root, string markupDirectory, int width, int height, bool startHidden = false) {
        this.root = root;
        NativeRuntime.Ensure(NativeRuntime.xr_layout(root, width, height) != 0);
        this.renderer = new AnglePreviewRenderer(markupDirectory, width, height);
        try {
            this.animations = NativeRuntime.xr_create_animation_controller();
            NativeRuntime.Ensure(this.animations != IntPtr.Zero);
            if (startHidden) {
                NativeRuntime.Ensure(NativeRuntime.xr_set_attribute(root, "visibility", "Collapsed") != 0);
            }
            NativeRuntime.Ensure(NativeRuntime.xr_attach_animations(root, this.animations) != 0);
            this.image = new Image {
                Width = this.renderer.Width,
                Height = this.renderer.Height,
                Stretch = Stretch.Fill
            };
            this.inspectionOutline = new Border {
                BorderBrush = Brushes.Red,
                BorderThickness = new Thickness(2.0),
                IsHitTestVisible = false,
                Visibility = Visibility.Collapsed
            };
            this.surface = new Grid {
                Width = this.renderer.Width,
                Height = this.renderer.Height
            };
            this.surface.Children.Add(this.image);
            this.surface.Children.Add(this.inspectionOutline);
            this.image.MouseLeftButtonDown += this.ImageMouseLeftButtonDown;
            this.image.MouseLeftButtonUp += this.ImageMouseLeftButtonUp;
            this.image.MouseMove += this.ImageMouseMove;
            this.image.MouseLeave += this.ImageMouseLeave;
            if (!startHidden) {
                this.Render();
            }
        } catch {
            if (this.animations != IntPtr.Zero) {
                NativeRuntime.xr_destroy_animation_controller(this.animations);
                this.animations = IntPtr.Zero;
            }
            this.renderer.Dispose();
            throw;
        }
    }

    public FrameworkElement Surface => this.surface;

    public event EventHandler? AnimationStarted;
    public event EventHandler<string>? Tapped;
    public event EventHandler<(int Line, int Column)>? ElementSelected;

    public void SetSourceLocations(IReadOnlyDictionary<IntPtr, (int Line, int Column)> locations) {
        this.sourceLocations = locations;
    }

    public void Transition(string from, string to, bool backward, bool visible) {
        this.ThrowIfDisposed();
        this.capturedElement = IntPtr.Zero;
        this.image.ReleaseMouseCapture();
        this.inspectionOutline.Visibility = Visibility.Collapsed;
        NativeRuntime.Ensure(NativeRuntime.xr_set_page_transition(
            this.root, from, to, backward ? 1 : 0, visible ? 1 : 0) != 0);
        this.Render();
    }

    public void SetAnimationSpeed(double value) {
        this.ThrowIfDisposed();
        NativeRuntime.Ensure(NativeRuntime.xr_set_animation_playback_rate(this.animations, (float)value) != 0);
    }

    public bool SetElementVisibility(string elementId, bool isVisible) {
        this.ThrowIfDisposed();
        var element = NativeRuntime.xr_find_element(this.root, elementId);
        if (element == IntPtr.Zero) {
            return false;
        }
        NativeRuntime.Ensure(NativeRuntime.xr_set_attribute(
            element, "visibility", isVisible ? "Visible" : "Collapsed") != 0);
        NativeRuntime.Ensure(NativeRuntime.xr_layout(this.root, this.renderer.Width, this.renderer.Height) != 0);
        this.Render();
        this.AnimationStarted?.Invoke(this, EventArgs.Empty);
        return true;
    }

    public void SetElementInspectionEnabled(bool value) {
        this.ThrowIfDisposed();
        if (this.isElementInspectionEnabled == value) {
            return;
        }
        this.isElementInspectionEnabled = value;
        this.image.Cursor = null;
        if (value) {
            this.capturedElement = IntPtr.Zero;
            this.image.ReleaseMouseCapture();
        }
        if (!value) {
            this.inspectionOutline.Visibility = Visibility.Collapsed;
            return;
        }
        if (this.image.IsMouseOver) {
            this.UpdateInspectionOutline(Mouse.GetPosition(this.image));
        }
    }

    public bool Update() {
        this.ThrowIfDisposed();
        var isAnimating = NativeRuntime.xr_update_animations(this.animations);
        NativeRuntime.Ensure(isAnimating >= 0);
        if (isAnimating == 0) {
            return false;
        }
        NativeRuntime.Ensure(NativeRuntime.xr_layout(this.root, this.renderer.Width, this.renderer.Height) != 0);
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
        if (this.isElementInspectionEnabled) {
            this.InspectElement(point);
            eventArgs.Handled = true;
            return;
        }
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
        if (this.isElementInspectionEnabled) {
            this.capturedElement = IntPtr.Zero;
            this.image.ReleaseMouseCapture();
            eventArgs.Handled = true;
            return;
        }
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
        this.UpdateInspectionOutline(point);
        if (this.isElementInspectionEnabled) {
            this.image.Cursor = Cursors.Cross;
            return;
        }
        var element = NativeRuntime.xr_hit_test(
            this.root,
            this.ScaleX(point.X),
            this.ScaleY(point.Y));
        this.image.Cursor = element == IntPtr.Zero ? null : Cursors.Hand;
    }

    private void ImageMouseLeave(object sender, MouseEventArgs eventArgs) {
        this.image.Cursor = null;
        this.inspectionOutline.Visibility = Visibility.Collapsed;
    }

    private void UpdateInspectionOutline(Point point) {
        if (!this.isElementInspectionEnabled || this.image.ActualWidth <= 0.0 || this.image.ActualHeight <= 0.0) {
            this.inspectionOutline.Visibility = Visibility.Collapsed;
            return;
        }
        var element = NativeRuntime.xr_hit_test_visual(
            this.root,
            this.ScaleX(point.X),
            this.ScaleY(point.Y));
        if (element == IntPtr.Zero || NativeRuntime.xr_element_bounds(element, out var bounds) == 0) {
            this.inspectionOutline.Visibility = Visibility.Collapsed;
            return;
        }
        this.inspectionOutline.Width = bounds.Width / this.renderer.Width * this.image.ActualWidth;
        this.inspectionOutline.Height = bounds.Height / this.renderer.Height * this.image.ActualHeight;
        this.inspectionOutline.HorizontalAlignment = HorizontalAlignment.Left;
        this.inspectionOutline.VerticalAlignment = VerticalAlignment.Top;
        this.inspectionOutline.Margin = new Thickness(
            bounds.X / this.renderer.Width * this.image.ActualWidth,
            bounds.Y / this.renderer.Height * this.image.ActualHeight,
            0.0,
            0.0);
        this.inspectionOutline.Visibility = Visibility.Visible;
    }

    private void InspectElement(Point point) {
        if (this.image.ActualWidth <= 0.0 || this.image.ActualHeight <= 0.0) {
            return;
        }
        var element = NativeRuntime.xr_hit_test_visual(this.root, this.ScaleX(point.X), this.ScaleY(point.Y));
        if (this.sourceLocations.TryGetValue(element, out var location)) {
            this.ElementSelected?.Invoke(this, location);
        }
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