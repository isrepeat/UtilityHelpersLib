#pragma once

#include "XamlRuntime/RenderEngine.h"

#include <cstddef>
#include <memory>

namespace es_renderer {
    class OpenGlRenderer final : public xaml::IRenderBackend {
    public:
        OpenGlRenderer(
            int width,
            int height,
            const unsigned char* fontData,
            size_t fontSize);
        ~OpenGlRenderer();

        OpenGlRenderer(const OpenGlRenderer&) = delete;
        OpenGlRenderer& operator=(const OpenGlRenderer&) = delete;

        void BeginFrame();

    private:
        //
        // IRenderBackend
        //
        void BeginClip(const xaml::Rect& bounds) override;
        void EndClip() override;
        void DrawOutline(const xaml::Rect& bounds, xaml::attr::Color color) override;
        void DrawRoundedRect(
            const xaml::Rect& bounds,
            xaml::attr::Color color,
            float cornerRadius) override;
        void DrawRoundedRectOutline(
            const xaml::Rect& bounds,
            xaml::attr::Color color,
            float cornerRadius,
            float thickness) override;
        void DrawText(
            const xaml::Rect& bounds,
            std::string_view text,
            xaml::attr::Color color,
            float fontSize,
            std::string_view fontWeight) override;
        void DrawImage(
            const xaml::Rect& bounds,
            std::string_view source,
            xaml::attr::Color tint) override;

        class Implementation;

    private:
        std::unique_ptr<Implementation> implementation;
    };
}