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
    private readonly PreviewCursorSet cursorSet;
    private IntPtr root;
    private IntPtr animations;
    private IntPtr capturedElement;
    private Point pointerDownPoint;
    private Point lastPointerPoint;
    private (IntPtr Element, string ElementId)? pendingSwipe;
    private bool isSwipeRemovalPending;
    private bool isElementInspectionEnabled;
    private GestureAxis gestureAxis;
    private bool isDisposed;
    private IReadOnlyDictionary<IntPtr, (int Line, int Column)> sourceLocations =
        new Dictionary<IntPtr, (int Line, int Column)>();

    public PreviewSession(
        IntPtr root,
        string markupDirectory,
        int width,
        int height,
        PreviewCursorSet cursorSet,
        bool startHidden = false) {
        this.root = root;
        this.cursorSet = cursorSet;
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
            this.image.MouseWheel += this.ImageMouseWheel;
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
    public event EventHandler<(IntPtr Element, string ElementId)>? Swiped;
    public event EventHandler<(int Line, int Column)>? ElementSelected;

    public void SetSourceLocations(IReadOnlyDictionary<IntPtr, (int Line, int Column)> locations) {
        this.sourceLocations = locations;
    }

    public bool TryGetElementBounds(string elementId, out NativeRect bounds) {
        this.ThrowIfDisposed();
        bounds = default;
        var element = NativeRuntime.xr_find_element(this.root, elementId);
        return element != IntPtr.Zero && NativeRuntime.xr_element_bounds(element, out bounds) != 0;
    }

    public IntPtr CaptureListRemovalTransition(IntPtr source) {
        this.ThrowIfDisposed();
        return NativeRuntime.xr_capture_list_removal_transition(source);
    }

    public int GetListItemIndex(IntPtr transition) {
        this.ThrowIfDisposed();
        return NativeRuntime.xr_list_removal_transition_item_index(transition);
    }

    public void ApplyListRemovalTransition(IntPtr transition) {
        this.ThrowIfDisposed();
        try {
            NativeRuntime.Ensure(NativeRuntime.xr_restore_list_removal_transition_offsets(this.root, transition) != 0);
            NativeRuntime.Ensure(NativeRuntime.xr_layout(this.root, this.renderer.Width, this.renderer.Height) != 0);
            NativeRuntime.Ensure(NativeRuntime.xr_animate_list_removal_transition(
                this.root,
                transition,
                this.animations,
                840) != 0);
            this.Render();
            this.AnimationStarted?.Invoke(this, EventArgs.Empty);
        } finally {
            NativeRuntime.xr_destroy_list_removal_transition(transition);
        }
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

    public bool SetElementAttribute(string elementId, string name, string value) {
        this.ThrowIfDisposed();
        var element = NativeRuntime.xr_find_element(this.root, elementId);
        if (element == IntPtr.Zero) {
            return false;
        }
        NativeRuntime.Ensure(NativeRuntime.xr_set_attribute(element, name, value) != 0);
        NativeRuntime.Ensure(NativeRuntime.xr_layout(this.root, this.renderer.Width, this.renderer.Height) != 0);
        this.Render();
        return true;
    }

    public bool GoToVisualState(string scopeId, string groupName, string stateName) {
        this.ThrowIfDisposed();
        var scope = NativeRuntime.xr_find_element(this.root, scopeId);
        if (scope == IntPtr.Zero) {
            return false;
        }
        NativeRuntime.Ensure(NativeRuntime.xr_go_to_visual_state(scope, groupName, stateName, 1) != 0);
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
        var isNativeAnimating = NativeRuntime.xr_update_animations(this.animations);
        NativeRuntime.Ensure(isNativeAnimating >= 0);
        if (isNativeAnimating == 0) {
            if (this.pendingSwipe is { } swipe) {
                this.pendingSwipe = null;
                this.isSwipeRemovalPending = false;
                NativeRuntime.xr_log_info($"Alarm swipe animation completed; element='{swipe.ElementId}'.");
                this.Swiped?.Invoke(this, swipe);
            }
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
        this.image.MouseWheel -= this.ImageMouseWheel;
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
        if (this.isSwipeRemovalPending) {
            return;
        }
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
        if (this.capturedElement == IntPtr.Zero) {
            this.capturedElement = NativeRuntime.xr_hit_test_visual(
                this.root,
                this.ScaleX(point.X),
                this.ScaleY(point.Y));
        }
        if (this.capturedElement != IntPtr.Zero) {
            var cursorKind = this.GetCursorKind(point);
            this.pointerDownPoint = new Point(this.ScaleX(point.X), this.ScaleY(point.Y));
            this.lastPointerPoint = this.pointerDownPoint;
            this.gestureAxis = GestureAxis.None;
            this.image.CaptureMouse();
            this.image.Cursor = cursorKind == PreviewCursorKind.Tap
                ? this.cursorSet.TapPressed
                : null;
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
        var capturedElement = this.capturedElement;
        var elementId = NativeRuntime.GetElementId(capturedElement);
        var point = eventArgs.GetPosition(this.image);
        var pointerUpPoint = new Point(this.ScaleX(point.X), this.ScaleY(point.Y));
        var horizontalDistance = pointerUpPoint.X - this.pointerDownPoint.X;
        if (this.gestureAxis != GestureAxis.Vertical) {
            NativeRuntime.Ensure(NativeRuntime.xr_handle_pointer_up(capturedElement, this.animations) != 0);
        }
        var isSwipe = this.gestureAxis == GestureAxis.Horizontal && Math.Abs(horizontalDistance) >= 180.0;
        this.capturedElement = IntPtr.Zero;
        this.image.ReleaseMouseCapture();
        this.SetCursor(this.GetCursorKind(point));
        this.Render();
        this.AnimationStarted?.Invoke(this, EventArgs.Empty);
        if (isSwipe) {
            NativeRuntime.Ensure(NativeRuntime.xr_animate_render_offset_x(
                capturedElement,
                this.animations,
                horizontalDistance < 0.0 ? -this.renderer.Width : this.renderer.Width,
                220) != 0);
            this.pendingSwipe = (capturedElement, elementId);
            this.isSwipeRemovalPending = true;
            NativeRuntime.xr_log_info($"Alarm swipe animation started; direction={(horizontalDistance < 0.0 ? "left" : "right")}, threshold=180.");
            this.AnimationStarted?.Invoke(this, EventArgs.Empty);
        } else if (this.gestureAxis != GestureAxis.Vertical) {
            NativeRuntime.xr_log_info($"Alarm swipe reset started; element='{elementId}', offset={horizontalDistance:0.0}, target=0, duration=180.");
            NativeRuntime.Ensure(NativeRuntime.xr_animate_render_offset_x(
                capturedElement,
                this.animations,
                0.0f,
                180) != 0);
            this.Tapped?.Invoke(this, elementId);
            this.AnimationStarted?.Invoke(this, EventArgs.Empty);
        } else if (elementId == "alarmBlock") {
            NativeRuntime.xr_log_info("Alarm swipe reset after scroll gesture started; target=0, duration=180.");
            NativeRuntime.Ensure(NativeRuntime.xr_animate_render_offset_x(
                capturedElement,
                this.animations,
                0.0f,
                180) != 0);
            this.AnimationStarted?.Invoke(this, EventArgs.Empty);
        }
        if (this.gestureAxis == GestureAxis.Vertical) {
            NativeRuntime.xr_scroll_end(this.animations);
            NativeRuntime.xr_log_info("Preview scroll drag completed; inertial update may continue.");
            this.AnimationStarted?.Invoke(this, EventArgs.Empty);
        }
        this.gestureAxis = GestureAxis.None;
        eventArgs.Handled = true;
    }

    private void ImageMouseMove(object sender, MouseEventArgs eventArgs) {
        if (this.capturedElement != IntPtr.Zero) {
            var pointerPoint = eventArgs.GetPosition(this.image);
            var horizontalDistance = this.ScaleX(pointerPoint.X) - this.pointerDownPoint.X;
            var verticalDistance = this.ScaleY(pointerPoint.Y) - this.pointerDownPoint.Y;
            if (this.gestureAxis == GestureAxis.None
                && Math.Max(Math.Abs(horizontalDistance), Math.Abs(verticalDistance)) >= 8.0) {
                if (Math.Abs(verticalDistance) > Math.Abs(horizontalDistance)) {
                    if (NativeRuntime.xr_scroll_begin(
                        this.root,
                        this.animations,
                        (float)this.pointerDownPoint.X,
                        (float)this.pointerDownPoint.Y) != 0) {
                        this.gestureAxis = GestureAxis.Vertical;
                    }
                } else if (NativeRuntime.GetElementId(this.capturedElement) == "alarmBlock") {
                    this.gestureAxis = GestureAxis.Horizontal;
                }
                if (this.gestureAxis != GestureAxis.None) {
                    NativeRuntime.xr_log_info($"Preview gesture axis locked: {this.gestureAxis}.");
                }
            }
            if (this.gestureAxis == GestureAxis.Vertical) {
                var verticalDelta = this.lastPointerPoint.Y - this.ScaleY(pointerPoint.Y);
                var didScroll = NativeRuntime.xr_scroll_drag(this.animations, (float)verticalDelta) != 0;
                this.lastPointerPoint = new Point(this.ScaleX(pointerPoint.X), this.ScaleY(pointerPoint.Y));
                if (didScroll) {
                    NativeRuntime.xr_log_info($"Preview scroll drag: verticalDelta={verticalDelta:0.0}.");
                    NativeRuntime.Ensure(NativeRuntime.xr_layout(this.root, this.renderer.Width, this.renderer.Height) != 0);
                    this.Render();
                }
                return;
            }
            if (this.gestureAxis == GestureAxis.Horizontal) {
                NativeRuntime.xr_log_info($"Alarm swipe drag: horizontalOffset={horizontalDistance:0.0}.");
                NativeRuntime.Ensure(NativeRuntime.xr_set_render_offset_x(
                    this.capturedElement,
                    (float)horizontalDistance) != 0);
                this.Render();
            }
            return;
        }
        var point = eventArgs.GetPosition(this.image);
        this.UpdateInspectionOutline(point);
        if (this.isElementInspectionEnabled) {
            this.image.Cursor = Cursors.Cross;
            return;
        }
        this.SetCursor(this.GetCursorKind(point));
    }

    private void ImageMouseLeave(object sender, MouseEventArgs eventArgs) {
        this.image.Cursor = null;
        this.inspectionOutline.Visibility = Visibility.Collapsed;
    }

    private PreviewCursorKind GetCursorKind(Point point) {
        return (PreviewCursorKind)NativeRuntime.xr_hit_test_cursor_kind(
            this.root,
            this.ScaleX(point.X),
            this.ScaleY(point.Y));
    }

    private void SetCursor(PreviewCursorKind kind) {
        this.image.Cursor = kind switch {
            PreviewCursorKind.Tap => this.cursorSet.Tap,
            _ => null,
        };
    }

    private enum GestureAxis {
        None,
        Horizontal,
        Vertical,
    }

    private enum PreviewCursorKind {
        None,
        Tap,
        Grab,
    }

    private void ImageMouseWheel(object sender, MouseWheelEventArgs eventArgs) {
        var point = eventArgs.GetPosition(this.image);
        if (NativeRuntime.xr_scroll_by(this.root, this.ScaleX(point.X), this.ScaleY(point.Y), 0.0f, -eventArgs.Delta / 120.0f * 72.0f) == 0) {
            return;
        }
        NativeRuntime.Ensure(NativeRuntime.xr_layout(this.root, this.renderer.Width, this.renderer.Height) != 0);
        this.Render();
        eventArgs.Handled = true;
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