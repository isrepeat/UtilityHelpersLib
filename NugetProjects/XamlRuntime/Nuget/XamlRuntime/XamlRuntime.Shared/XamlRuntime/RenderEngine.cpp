#include "XamlRuntime/RenderEngine.h"

#include <algorithm>
#include <cmath>

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

    void RenderButtonWave(
        const Element& element,
        IRenderBackend& backend,
        Rect bounds,
        float opacity) {
        const float progress = element.WaveProgress();
        if (progress < 0.0f || element.WaveOpacity() <= 0.0f) {
            return;
        }

        const float fade = std::pow(element.WaveOpacity(), element.WaveFadeExponent());
        const attr::Color color{
            element.Foreground().red,
            element.Foreground().green,
            element.Foreground().blue,
            element.Foreground().alpha * fade * element.WaveIntensity(),
        };
        const attr::Color waveColor = WithOpacity(color, opacity);
        backend.DrawShader(
            "button-wave",
            bounds,
            {
                {"cornerRadius", {element.CornerRadius()}, 1},
                {"progress", {std::min(progress, 1.0f)}, 1},
                {"spread", {element.WaveSpread()}, 1},
                {"rippleColor", {
                    waveColor.red,
                    waveColor.green,
                    waveColor.blue,
                    waveColor.alpha,
                }, 4},
            });
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
        float inheritedOpacity);

    void RenderDefaultElement(
        const Element& element,
        IRenderBackend& backend,
        const RendererRegistry* renderers,
        Rect bounds,
        float offsetX,
        float opacity) {
        RenderChrome(element, backend, bounds, opacity);
        if (element.Type() == ElementType::button) {
            RenderButtonWave(element, backend, bounds, opacity);
        }
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

        for (const auto& child : element.Children()) {
            RenderElement(*child, backend, renderers, offsetX, opacity);
        }
    }

    void RenderElement(
        const Element& element,
        IRenderBackend& backend,
        const RendererRegistry* renderers,
        float inheritedOffsetX,
        float inheritedOpacity) {
        if (element.VisibilityValue() != attr::Visibility::visible) {
            return;
        }

        const float offsetX = inheritedOffsetX + element.RenderOffsetX();
        const float opacity = inheritedOpacity * element.Opacity();
        Rect bounds = Translate(element.Bounds(), offsetX);
        backend.BeginClip(Translate(element.ClipBounds(), offsetX));
        RenderContext context(
            backend,
            bounds,
            opacity,
            [&element, &backend, renderers, bounds, offsetX, opacity]() {
                RenderDefaultElement(element, backend, renderers, bounds, offsetX, opacity);
            });
        if (renderers != nullptr && renderers->Render(element, context)) {
            backend.EndClip();
            return;
        }

        context.RenderDefaultElement();
        backend.EndClip();
    }
}

namespace xaml {
    RenderContext::RenderContext(
        IRenderBackend& backend,
        const Rect& bounds,
        float opacity,
        std::function<void()> defaultElementRenderer)
        : backend(backend)
        , bounds(bounds)
        , opacity(opacity)
        , defaultElementRenderer(std::move(defaultElementRenderer)) {
    }

    IRenderBackend& RenderContext::Backend() {
        return this->backend;
    }

    const Rect& RenderContext::Bounds() const {
        return this->bounds;
    }

    float RenderContext::Opacity() const {
        return this->opacity;
    }

    void RenderContext::RenderDefaultElement() {
        if (this->defaultElementRendered) {
            return;
        }

        this->defaultElementRendered = true;
        this->defaultElementRenderer();
    }

    void RendererRegistry::Register(std::string name, ElementRenderer renderer) {
        this->renderers.insert_or_assign(std::move(name), std::move(renderer));
    }

    bool RendererRegistry::Render(const Element& element, RenderContext& context) const {
        const auto found = this->renderers.find(element.Renderer());
        return !element.Renderer().empty() && found != this->renderers.end()
            && found->second(element, context);
    }

    void Render(Element& root, IRenderBackend& backend) {
        if (root.layoutInvalid) {
            layout(root, root.availableSize);
        }
        _details::RenderElement(root, backend, nullptr, 0.0f, 1.0f);
    }

    void Render(Element& root, IRenderBackend& backend, const RendererRegistry& renderers) {
        if (root.layoutInvalid) {
            layout(root, root.availableSize);
        }
        _details::RenderElement(root, backend, &renderers, 0.0f, 1.0f);
    }
}