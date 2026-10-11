#pragma once
#include "../../AndroidAppPreviewerPlugin.h"

namespace preview_sdk::abi {
    using namespace AndroidAppPreviewerPluginSDK;

    class RenderingAbi final {
    public:
        static xp_angle_surface* xp_create_angle_surface(
            int width,
            int height,
            const char* fontPath,
            const char* resourceRoot
        );
        static void xp_destroy_angle_surface(xp_angle_surface* surface);
        static int xp_render_angle_surface(
            xp_angle_surface* surface,
            const xp_element* root,
            unsigned char* destination,
            int destinationStride,
            int destinationCapacity
        );
        static int xp_render(
            const xp_element* root,
            xp_command* destination,
            int capacity
        );
        static int xp_render_angle(
            const xp_element* root,
            const char* fontPath,
            int width,
            int height,
            const char* resourceRoot,
            unsigned char* destination,
            int destinationStride,
            int destinationCapacity
        );
    };
}