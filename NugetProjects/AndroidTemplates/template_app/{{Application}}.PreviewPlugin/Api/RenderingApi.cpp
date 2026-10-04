#include "RenderingApi.h"

#include <XamlRuntime/RenderEngine.h>

#include "../Rendering/RecordingBackend.h"
#include "../Bridge/PreviewPluginSdkTypes.h"
#include "../Bridge/Diagnostic.h"

#include <algorithm>
#include <stdexcept>

namespace {{application}}::preview::api {
    using namespace AndroidAppPreviewerPluginSDK;

    //
    // API
    //
    xp_angle_surface* RenderingApi::xp_create_angle_surface(
        int width,
        int height,
        const char* fontPath,
        const char* resourceRoot
    ) {
        try {
            {{application}}::preview::bridge::LastError().clear();
            if (fontPath == nullptr || resourceRoot == nullptr) {
                throw std::invalid_argument("fontPath and resourceRoot are required");
            }
            return new xp_angle_surface(width, height, fontPath, resourceRoot);
        } catch (const std::exception& error) {
            {{application}}::preview::bridge::LastError() = error.what();
            return nullptr;
        }
    }

    void RenderingApi::xp_destroy_angle_surface(xp_angle_surface* surface) {
        delete surface;
    }

    int RenderingApi::xp_render_angle_surface(
        xp_angle_surface* surface,
        const xp_element* root,
        unsigned char* destination,
        int destinationStride,
        int destinationCapacity
    ) {
        try {
            {{application}}::preview::bridge::LastError().clear();
            if (surface == nullptr || root == nullptr || destination == nullptr
                || destinationStride < surface->width * 4
                || destinationCapacity / destinationStride < surface->height) {
                throw std::invalid_argument("Invalid persistent ANGLE render arguments");
            }
            surface->value.Render(
                *reinterpret_cast<xaml::Element*>(const_cast<xp_element*>(root)),
                destination,
                destinationStride);
            return 1;
        } catch (const std::exception& error) {
            {{application}}::preview::bridge::LastError() = error.what();
            return 0;
        }
    }

    int RenderingApi::xp_render(
        const xp_element* root,
        xp_command* destination,
        int capacity
    ) {
        try {
            {{application}}::preview::bridge::LastError().clear();
            if (root == nullptr) {
                throw std::invalid_argument("root is required");
            }
            {{application}}::preview::rendering::RecordingBackend backend;
            xaml::Render(*reinterpret_cast<xaml::Element*>(const_cast<xp_element*>(root)), backend);
            const auto& commands = backend.Commands();
            if (destination != nullptr && capacity > 0) {
                const size_t count = std::min(commands.size(), static_cast<size_t>(capacity));
                std::copy_n(commands.data(), count, destination);
            }
            return static_cast<int>(commands.size());
        } catch (const std::exception& error) {
            {{application}}::preview::bridge::LastError() = error.what();
            return -1;
        }
    }

    int RenderingApi::xp_render_angle(
        const xp_element* root,
        const char* fontPath,
        int width,
        int height,
        const char* resourceRoot,
        unsigned char* destination,
        int destinationStride,
        int destinationCapacity
    ) {
        try {
            {{application}}::preview::bridge::LastError().clear();
            if (root == nullptr || fontPath == nullptr || resourceRoot == nullptr || destination == nullptr
                || width <= 0 || height <= 0 || destinationStride < width * 4
                || destinationCapacity / destinationStride < height) {
                throw std::invalid_argument("Invalid ANGLE render arguments");
            }

            {{application}}::preview::rendering::AngleRenderSurface surface(width, height, fontPath, resourceRoot);
            surface.Render(
                *reinterpret_cast<xaml::Element*>(const_cast<xp_element*>(root)),
                destination,
                destinationStride);
            return 1;
        } catch (const std::exception& error) {
            {{application}}::preview::bridge::LastError() = error.what();
            return 0;
        }
    }
} // namespace {{application}}::preview::api