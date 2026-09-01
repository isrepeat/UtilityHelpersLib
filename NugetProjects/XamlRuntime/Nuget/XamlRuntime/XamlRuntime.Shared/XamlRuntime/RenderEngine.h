#pragma once

#include "XamlRuntime/XamlLayout.h"

#include <string_view>

namespace xaml {
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
        virtual void DrawText(
            const Rect& bounds,
            std::string_view text,
            attr::Color color,
            float fontSize,
            std::string_view fontWeight) = 0;
        virtual void DrawImage(
            const Rect& bounds,
            std::string_view source,
            attr::Color tint) = 0;
    };

    void Render(const Element& root, IRenderBackend& backend);
}