#include "XamlRuntime/RenderEngine.h"

#include <algorithm>

namespace xaml::_details {
    Rect Translate(Rect bounds, float offsetX) {
        bounds.x += offsetX;
        return bounds;
    }

    attr::Color WithOpacity(attr::Color color, float opacity) {
        color.alpha *= opacity;
        return color;
    }

    attr::Color InterpolateColor(attr::Color from, attr::Color to, float progress) {
        return {
            from.red + (to.red - from.red) * progress,
            from.green + (to.green - from.green) * progress,
            from.blue + (to.blue - from.blue) * progress,
            from.alpha + (to.alpha - from.alpha) * progress,
        };
    }

    Rect ScaleAroundCenter(Rect bounds, float scale) {
        const float width = bounds.width * scale;
        const float height = bounds.height * scale;
        return {
            bounds.x + (bounds.width - width) / 2.0f,
            bounds.y + (bounds.height - height) / 2.0f,
            width,
            height,
        };
    }

    attr::Color ButtonColor(
        const Element& element,
        attr::Color inactive,
        attr::Color active) {
        return element.IsOn() && active.alpha > 0.0f ? active : inactive;
    }

    void RenderToggleSwitch(
        const Element& element,
        IRenderBackend& backend,
        Rect bounds,
        float opacity) {
        const float targetProgress = element.IsOn() ? 1.0f : 0.0f;
        const float progress = element.ToggleProgress() < 0.0f
            ? targetProgress : element.ToggleProgress();
        backend.DrawRoundedRect(
            bounds,
            WithOpacity(
                InterpolateColor(element.BorderColor(), element.Background(), progress),
                opacity),
            bounds.height / 2.0f);
        const float inset = 4.0f;
        const float thumbSize = std::max(0.0f, bounds.height - inset * 2.0f);
        const float thumbX = bounds.x + inset
            + (bounds.width - inset * 2.0f - thumbSize) * progress;
        backend.DrawRoundedRect(
            {thumbX, bounds.y + inset, thumbSize, thumbSize},
            WithOpacity(
                InterpolateColor(element.Tint(), element.Foreground(), progress),
                opacity),
            thumbSize / 2.0f);
    }

    void RenderSvgImage(const Element& element, IRenderBackend& backend) {
        backend.DrawImage(element.Bounds(), element.Source(), element.Tint());
    }

    void RenderChrome(
        const Element& element,
        IRenderBackend& backend,
        Rect bounds,
        float opacity) {
        if (element.Type() == ElementType::toggleSwitch) {
            return;
        }

        const attr::Color background = element.Type() == ElementType::button
            ? ButtonColor(element, element.Background(), element.ActiveBackground())
            : element.Background();
        const attr::Color borderColor = element.Type() == ElementType::button
            ? ButtonColor(element, element.BorderColor(), element.ActiveBorderColor())
            : element.BorderColor();
        const attr::Thickness borderThickness = element.BorderThickness();
        const float thickness = std::max({
            borderThickness.left,
            borderThickness.right,
            borderThickness.top,
            borderThickness.bottom,
        });
        if (thickness <= 0.0f) {
            backend.DrawRoundedRect(
                bounds,
                WithOpacity(background, opacity),
                element.CornerRadius());
            return;
        }
        if (background.alpha <= 0.0f) {
            backend.DrawRoundedRectOutline(
                bounds,
                WithOpacity(borderColor, opacity),
                element.CornerRadius(),
                thickness);
            return;
        }

        backend.DrawRoundedRect(
            bounds,
            WithOpacity(borderColor, opacity),
            element.CornerRadius());
        const Rect innerBounds{
            bounds.x + borderThickness.left,
            bounds.y + borderThickness.top,
            std::max(0.0f, bounds.width - borderThickness.left - borderThickness.right),
            std::max(0.0f, bounds.height - borderThickness.top - borderThickness.bottom),
        };
        backend.DrawRoundedRect(
            innerBounds,
            WithOpacity(background, opacity),
            std::max(0.0f, element.CornerRadius() - thickness));
    }

    void RenderElement(
        const Element& element,
        IRenderBackend& backend,
        float inheritedOffsetX,
        float inheritedOpacity) {
        if (element.VisibilityValue() != attr::Visibility::visible) {
            return;
        }

        const float offsetX = inheritedOffsetX + element.RenderOffsetX();
        const float opacity = inheritedOpacity * element.Opacity();
        Rect bounds = Translate(element.Bounds(), offsetX);
        if (element.Type() == ElementType::button) {
            bounds = ScaleAroundCenter(bounds, 1.0f - element.PressProgress() * 0.06f);
        }
        backend.BeginClip(Translate(element.ClipBounds(), offsetX));
        RenderChrome(element, backend, bounds, opacity);
        if (element.Type() == ElementType::textBlock) {
            backend.DrawText(
                bounds,
                element.Text(),
                WithOpacity(element.Foreground(), opacity),
                element.FontSize(),
                element.FontWeight(),
                element.HorizontalAlignmentValue());
        } else if (element.Type() == ElementType::button
            || element.Type() == ElementType::iconButton) {
            const attr::Color foreground = element.Type() == ElementType::button
                ? ButtonColor(element, element.Foreground(), element.ActiveForeground())
                : element.Foreground();
            backend.DrawOutline(bounds, WithOpacity(foreground, opacity));
            backend.DrawText(
                bounds,
                element.Text(),
                WithOpacity(foreground, opacity),
                element.FontSize(),
                element.FontWeight(),
                element.HorizontalAlignmentValue());
        } else if (element.Type() == ElementType::toggleSwitch) {
            RenderToggleSwitch(element, backend, bounds, opacity);
        } else if (element.Type() == ElementType::image) {
            backend.DrawImage(bounds, element.Source(), WithOpacity(element.Tint(), opacity));
        } else if (element.Type() == ElementType::svgImage) {
            backend.DrawImage(bounds, element.Source(), WithOpacity(element.Tint(), opacity));
        }

        for (const auto& child : element.Children()) {
            RenderElement(*child, backend, offsetX, opacity);
        }
        backend.EndClip();
    }
}

namespace xaml {
    void Render(Element& root, IRenderBackend& backend) {
        if (root.layoutInvalid) {
            layout(root, root.availableSize);
        }
        _details::RenderElement(root, backend, 0.0f, 1.0f);
    }
}