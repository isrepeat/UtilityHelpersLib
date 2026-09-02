#include "XamlRuntime/RenderEngine.h"

#include <algorithm>

namespace xaml::_details {
    void RenderToggleSwitch(const Element& element, IRenderBackend& backend) {
        const Rect bounds = element.Bounds();
        const bool isOn = element.IsOn();
        backend.DrawRoundedRect(
            bounds,
            isOn ? element.Background() : element.BorderColor(),
            bounds.height / 2.0f);
        const float inset = 4.0f;
        const float thumbSize = std::max(0.0f, bounds.height - inset * 2.0f);
        const float thumbX = isOn
            ? bounds.x + bounds.width - inset - thumbSize
            : bounds.x + inset;
        backend.DrawRoundedRect(
            {thumbX, bounds.y + inset, thumbSize, thumbSize},
            isOn ? element.Foreground() : element.Tint(),
            thumbSize / 2.0f);
    }

    void RenderSvgImage(const Element& element, IRenderBackend& backend) {
        const Rect bounds = element.Bounds();
        const attr::Color tint = element.Tint();
        if (element.Source() == "Icons/add.svg") {
            const float thickness = std::max(2.0f, bounds.width * 0.12f);
            backend.DrawRoundedRect(
                {bounds.x, bounds.y + (bounds.height - thickness) / 2.0f, bounds.width, thickness},
                tint,
                thickness / 2.0f);
            backend.DrawRoundedRect(
                {bounds.x + (bounds.width - thickness) / 2.0f, bounds.y, thickness, bounds.height},
                tint,
                thickness / 2.0f);
            return;
        }
        if (element.Source() == "Icons/settings.svg") {
            const float centerX = bounds.x + bounds.width / 2.0f;
            const float centerY = bounds.y + bounds.height / 2.0f;
            backend.DrawRoundedRect(bounds, tint, bounds.width / 2.0f);
            const float hole = bounds.width * 0.34f;
            backend.DrawRoundedRect(
                {centerX - hole / 2.0f, centerY - hole / 2.0f, hole, hole},
                {0.16f, 0.164f, 0.137f, 1.0f},
                hole / 2.0f);
        }
    }

    void RenderChrome(const Element& element, IRenderBackend& backend) {
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
                element.Bounds(),
                element.Background(),
                element.CornerRadius());
            return;
        }
        if (element.Background().alpha <= 0.0f) {
            backend.DrawRoundedRectOutline(
                element.Bounds(),
                element.BorderColor(),
                element.CornerRadius(),
                thickness);
            return;
        }

        const Rect bounds = element.Bounds();
        backend.DrawRoundedRect(bounds, element.BorderColor(), element.CornerRadius());
        const Rect innerBounds{
            bounds.x + borderThickness.left,
            bounds.y + borderThickness.top,
            std::max(0.0f, bounds.width - borderThickness.left - borderThickness.right),
            std::max(0.0f, bounds.height - borderThickness.top - borderThickness.bottom),
        };
        backend.DrawRoundedRect(
            innerBounds,
            element.Background(),
            std::max(0.0f, element.CornerRadius() - thickness));
    }

    void RenderElement(const Element& element, IRenderBackend& backend) {
        if (element.VisibilityValue() != attr::Visibility::visible) {
            return;
        }

        backend.BeginClip(element.ClipBounds());
        RenderChrome(element, backend);
        if (element.Type() == ElementType::textBlock) {
            backend.DrawText(
                element.Bounds(),
                element.Text(),
                element.Foreground(),
                element.FontSize(),
                element.FontWeight());
        } else if (element.Type() == ElementType::button
            || element.Type() == ElementType::iconButton) {
            backend.DrawOutline(element.Bounds(), element.Foreground());
            backend.DrawText(
                element.Bounds(),
                element.Text(),
                element.Foreground(),
                element.FontSize(),
                element.FontWeight());
        } else if (element.Type() == ElementType::toggleSwitch) {
            RenderToggleSwitch(element, backend);
        } else if (element.Type() == ElementType::image) {
            backend.DrawImage(element.Bounds(), element.Source(), element.Tint());
        } else if (element.Type() == ElementType::svgImage) {
            RenderSvgImage(element, backend);
        }

        for (const auto& child : element.Children()) {
            RenderElement(*child, backend);
        }
        backend.EndClip();
    }
}

namespace xaml {
    void Render(Element& root, IRenderBackend& backend) {
        if (root.layoutInvalid) {
            layout(root, root.availableSize);
        }
        _details::RenderElement(root, backend);
    }
}