#pragma once

#include "XamlRuntime/XamlLayout.h"

#include <initializer_list>
#include <functional>
#include <unordered_map>
#include <string_view>

namespace xaml {
    struct ShaderUniform {
        std::string_view name;
        float values[4]{};
        int valueCount = 0;
    };

    class IRenderBackend {
    public:
        virtual ~IRenderBackend() = default;

        virtual void BeginClip(const Rect& bounds) = 0;
        virtual void EndClip() = 0;
        virtual void DrawOutline(const Rect& bounds, attr::Color color) = 0;
        virtual void DrawRoundedRect(
            const Rect& bounds,
            attr::Color color,
            float cornerRadius) = 0;
        virtual void DrawRoundedRectOutline(
            const Rect& bounds,
            attr::Color color,
            float cornerRadius,
            float thickness) = 0;
        virtual void DrawShader(
            std::string_view shaderName,
            const Rect& bounds,
            std::initializer_list<ShaderUniform> uniforms) = 0;
        virtual void DrawText(
            const Rect& bounds,
            std::string_view text,
            attr::Color color,
            float fontSize,
            std::string_view fontWeight,
            attr::Alignment horizontalAlignment) = 0;
        virtual void DrawImage(
            const Rect& bounds,
            std::string_view source,
            attr::Color tint) = 0;
    };

    class RenderContext {
    public:
        RenderContext(
            IRenderBackend& backend,
            const Rect& bounds,
            float opacity,
            std::function<void()> defaultElementRenderer);
        IRenderBackend& Backend();
        const Rect& Bounds() const;
        float Opacity() const;
        void RenderDefaultElement();

    private:
        IRenderBackend& backend;
        const Rect& bounds;
        float opacity;
        std::function<void()> defaultElementRenderer;
        bool defaultElementRendered = false;
    };

    class RendererRegistry {
    public:
        using ElementRenderer = std::function<bool(const Element&, RenderContext&)>;

        void Register(std::string name, ElementRenderer renderer);
        bool Render(const Element& element, RenderContext& context) const;

    private:
        std::unordered_map<std::string, ElementRenderer> renderers;
    };

    void Render(Element& root, IRenderBackend& backend);
    void Render(Element& root, IRenderBackend& backend, const RendererRegistry& renderers);
}