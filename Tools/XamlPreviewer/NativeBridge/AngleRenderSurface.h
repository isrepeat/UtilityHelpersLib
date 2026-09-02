#pragma once

#include "NativeBridge.h"

#include <memory>
#include <string_view>

namespace xaml::bridge {
    // Изолирует EGL pbuffer и OpenGL ES ресурсы от C ABI native bridge.
    class AngleRenderSurface {
    public:
        AngleRenderSurface(int width, int height, std::string_view fontPath);
        ~AngleRenderSurface();

        AngleRenderSurface(const AngleRenderSurface&) = delete;
        AngleRenderSurface& operator=(const AngleRenderSurface&) = delete;

        void Render(
            const xr_command* commands,
            int commandCount,
            unsigned char* destination,
            int destinationStride);

    private:
        class Implementation;

    private:
        std::unique_ptr<Implementation> implementation;
    };
}