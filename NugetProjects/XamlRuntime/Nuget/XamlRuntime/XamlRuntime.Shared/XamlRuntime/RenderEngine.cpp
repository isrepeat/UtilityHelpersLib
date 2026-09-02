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
                WithOpacity(element.Background(), opacity),
                element.CornerRadius());
            return;
        }
        if (element.Background().alpha <= 0.0f) {
            backend.DrawRoundedRectOutline(
                bounds,
                WithOpacity(element.BorderColor(), opacity),
                element.CornerRadius(),
                thickness);
            return;
        }

        backend.DrawRoundedRect(
            bounds,
            WithOpacity(element.BorderColor(), opacity),
            element.CornerRadius());
        const Rect innerBounds{
            bounds.x + borderThickness.left,
            bounds.y + borderThickness.top,
            std::max(0.0f, bounds.width - borderThickness.left - borderThickness.right),
            std::max(0.0f, bounds.height - borderThickness.top - borderThickness.bottom),
        };
        backend.DrawRoundedRect(
            innerBounds,
            WithOpacity(element.Background(), opacity),
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
        const Rect bounds = Translate(element.Bounds(), offsetX);
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
            backend.DrawOutline(bounds, WithOpacity(element.Foreground(), opacity));
            backend.DrawText(
                bounds,
                element.Text(),
                WithOpacity(element.Foreground(), opacity),
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