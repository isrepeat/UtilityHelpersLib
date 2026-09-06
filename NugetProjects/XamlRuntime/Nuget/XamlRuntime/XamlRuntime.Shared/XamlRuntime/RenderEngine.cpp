#include "XamlRuntime/RenderEngine.h"

#include <algorithm>
#include <cmath>

namespace xaml::_details {
    Rect Translate(Rect bounds, float offsetX, float offsetY = 0.0f) {
        bounds.x += offsetX;
        bounds.y += offsetY;
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

    bool AreEqual(attr::Color left, attr::Color right) {
        return left.red == right.red
            && left.green == right.green
            && left.blue == right.blue
            && left.alpha == right.alpha;
    }

    attr::Color ButtonColor(
        const Element& element,
        attr::Color inactive,
        attr::Color active) {
        if (active.alpha <= 0.0f) {
            return inactive;
        }
        return InterpolateColor(inactive, active, element.PressProgress());
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
        if (AreEqual(background, borderColor)) {
            backend.DrawRoundedRect(
                bounds,
                WithOpacity(background, opacity),
                element.CornerRadius());
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
        const RendererRegistry* renderers,
        float inheritedOffsetX,
        float inheritedOffsetY,
        float inheritedOpacity);

    void RenderDefaultElement(
        const Element& element,
        IRenderBackend& backend,
        Rect bounds,
        float opacity) {
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
            backend.DrawText(
                bounds,
                element.Text(),
                WithOpacity(foreground, opacity),
                element.FontSize(),
                element.FontWeight(),
                element.Type() == ElementType::button
                    ? element.ContentAlignmentValue()
                    : element.HorizontalAlignmentValue());
        } else if (element.Type() == ElementType::toggleSwitch) {
            RenderToggleSwitch(element, backend, bounds, opacity);
        } else if (element.Type() == ElementType::image) {
            backend.DrawImage(bounds, element.Source(), WithOpacity(element.Tint(), opacity));
        } else if (element.Type() == ElementType::svgImage) {
            backend.DrawImage(bounds, element.Source(), WithOpacity(element.Tint(), opacity));
        }
    }

    void RenderElement(
        const Element& element,
        IRenderBackend& backend,
        const RendererRegistry* renderers,
        float inheritedOffsetX,
        float inheritedOffsetY,
        float inheritedOpacity) {
        if (!element.IsPresent()) {
            return;
        }

        const VisualTransform defaults{};
        const auto& transform = element.States().Contains<VisualTransform>()
            ? element.State<VisualTransform>() : defaults;
        const float offsetX = inheritedOffsetX + element.RenderOffsetX() + transform.offsetX;
        const float offsetY = inheritedOffsetY + transform.offsetY;
        const float opacity = inheritedOpacity * element.Opacity() * transform.opacity;
        const Rect bounds = Translate(element.Bounds(), offsetX, offsetY);
        backend.BeginClip(Translate(element.ClipBounds(), offsetX, offsetY));
        RenderInvocation context(backend, bounds, opacity,
            [&element, &backend, bounds, opacity]() {
                RenderDefaultElement(element, backend, bounds, opacity);
            },
            [&element, &backend, renderers, offsetX, offsetY, opacity]() {
                for (const auto& child : element.Children()) {
                    RenderElement(*child, backend, renderers, offsetX, offsetY, opacity);
                }
            });
        try {
            if (renderers == nullptr || !renderers->Render(element, context)) {
                context.RenderDefault();
            }
        } catch (...) {
            backend.EndClip();
            throw;
        }
        backend.EndClip();
    }
}

namespace xaml {
    RenderInvocation::RenderInvocation(
        IRenderBackend& backend,
        const Rect& bounds,
        float opacity,
        std::function<void()> defaultElementRenderer,
        std::function<void()> childrenRenderer)
        : backend(backend)
        , bounds(bounds)
        , opacity(opacity)
        , defaultElementRenderer(std::move(defaultElementRenderer))
        , childrenRenderer(std::move(childrenRenderer)) {
    }

    IRenderBackend& RenderInvocation::Backend() {
        return this->backend;
    }

    const Rect& RenderInvocation::Bounds() const {
        return this->bounds;
    }

    float RenderInvocation::Opacity() const {
        return this->opacity;
    }

    void RenderInvocation::RenderDefaultElement() {
        if (this->defaultElementRendered) {
            return;
        }

        this->defaultElementRendered = true;
        this->defaultElementRenderer();
        this->RenderChildren();
    }

    void RenderInvocation::RenderDefault() {
        this->RenderDefaultElement();
    }

    void RenderInvocation::RenderChildren() {
        if (!this->childrenRendered && this->childrenRenderer) {
            this->childrenRendered = true;
            this->childrenRenderer();
        }
    }

    RendererRegistry::RendererRegistry(StateRegistry states)
        : states(std::move(states)) {
    }

    //
    // API
    //
    void RendererRegistry::Prepare(Element& root) const {
        const auto found = this->renderers.find(root.Renderer());
        if (found != this->renderers.end()) {
            root.States().Prepare(this->states, found->second.stateType);
        }
        for (const auto& child : root.Children()) {
            this->Prepare(*child);
        }
    }

    bool RendererRegistry::Render(const Element& element, RenderInvocation& context) const {
        const auto found = this->renderers.find(element.Renderer());
        return !element.Renderer().empty() && found != this->renderers.end()
            && found->second.render(element, context);
    }

    void Render(Element& root, IRenderBackend& backend) {
        if (root.layoutInvalid) {
            layout(root, root.availableSize);
        }
        _details::RenderElement(root, backend, nullptr, 0.0f, 0.0f, 1.0f);
    }

    void Render(Element& root, IRenderBackend& backend, const RendererRegistry& renderers) {
        renderers.Prepare(root);
        if (root.layoutInvalid) {
            layout(root, root.availableSize);
        }
        _details::RenderElement(root, backend, &renderers, 0.0f, 0.0f, 1.0f);
    }
}