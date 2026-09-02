#include "NativeBridge.h"
#include "AngleRenderSurface.h"

#include <XamlRuntime/ElementBuilder.h>
#include <XamlRuntime/RenderEngine.h>
#include <XamlRuntime/XamlLayout.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace xaml::bridge {
    class RecordingBackend final : public IRenderBackend {
    public:
        //
        // IRenderBackend
        //
        void BeginClip(const Rect& bounds) override {
            this->Append(xr_command_type_begin_clip, bounds);
        }

        void EndClip() override {
            this->Append(xr_command_type_end_clip, {});
        }

        void DrawOutline(const Rect& bounds, attr::Color color) override {
            this->Append(xr_command_type_outline, bounds, color);
        }

        void DrawRoundedRect(const Rect& bounds, attr::Color color, float cornerRadius) override {
            this->Append(xr_command_type_rounded_rect, bounds, color, cornerRadius);
        }

        void DrawRoundedRectOutline(
            const Rect& bounds,
            attr::Color color,
            float cornerRadius,
            float thickness) override {
            xr_command& command = this->Append(
                xr_command_type_rounded_rect_outline,
                bounds,
                color,
                cornerRadius);
            std::memcpy(command.auxiliary, &thickness, sizeof(thickness));
        }

        void DrawText(
            const Rect& bounds,
            std::string_view text,
            attr::Color color,
            float fontSize,
            std::string_view fontWeight) override {
            xr_command& command = this->Append(xr_command_type_text, bounds, color, fontSize);
            Copy(text, command.text, sizeof(command.text));
            Copy(fontWeight, command.auxiliary, sizeof(command.auxiliary));
        }

        void DrawImage(
            const Rect& bounds,
            std::string_view source,
            attr::Color tint) override {
            xr_command& command = this->Append(xr_command_type_image, bounds, tint);
            Copy(source, command.text, sizeof(command.text));
        }

        const std::vector<xr_command>& Commands() const {
            return this->commands;
        }

    private:
        xr_command& Append(
            xr_command_type type,
            const Rect& bounds,
            attr::Color color = {},
            float value = 0.0f) {
            this->commands.push_back({
                static_cast<int>(type),
                {bounds.x, bounds.y, bounds.width, bounds.height},
                {color.red, color.green, color.blue, color.alpha},
                value,
            });
            return this->commands.back();
        }

        static void Copy(std::string_view source, char* destination, size_t capacity) {
            const size_t length = std::min(source.size(), capacity - 1);
            std::memcpy(destination, source.data(), length);
            destination[length] = '\0';
        }

    private:
        std::vector<xr_command> commands;
    };

    thread_local std::string lastError;
}

const char* xr_last_error(void) {
    return xaml::bridge::lastError.c_str();
}

xr_element* xr_create_element(const char* type) {
    try {
        xaml::bridge::lastError.clear();
        return reinterpret_cast<xr_element*>(new xaml::Element(xaml::ParseElementType(type)));
    } catch (const std::exception& error) {
        xaml::bridge::lastError = error.what();
        return nullptr;
    }
}

void xr_destroy_element(xr_element* element) {
    delete reinterpret_cast<xaml::Element*>(element);
}

int xr_add_child(xr_element* parent, xr_element* child) {
    try {
        xaml::bridge::lastError.clear();
        if (parent == nullptr || child == nullptr) {
            throw std::invalid_argument("parent and child are required");
        }
        xaml::ValidateChild(
            *reinterpret_cast<const xaml::Element*>(parent),
            *reinterpret_cast<const xaml::Element*>(child));
        reinterpret_cast<xaml::Element*>(parent)->AddChild(
            std::unique_ptr<xaml::Element>(reinterpret_cast<xaml::Element*>(child)));
        return 1;
    } catch (const std::exception& error) {
        xaml::bridge::lastError = error.what();
        return 0;
    }
}

int xr_set_attribute(
    xr_element* element,
    const char* name,
    const char* value) {
    try {
        xaml::bridge::lastError.clear();
        if (element == nullptr) {
            throw std::invalid_argument("element is required");
        }
        xaml::SetAttribute(*reinterpret_cast<xaml::Element*>(element), name, value);
        return 1;
    } catch (const std::exception& error) {
        xaml::bridge::lastError = error.what();
        return 0;
    }
}

int xr_layout(xr_element* root, float width, float height) {
    try {
        xaml::bridge::lastError.clear();
        if (root == nullptr) {
            throw std::invalid_argument("root is required");
        }
        xaml::layout(*reinterpret_cast<xaml::Element*>(root), {width, height});
        return 1;
    } catch (const std::exception& error) {
        xaml::bridge::lastError = error.what();
        return 0;
    }
}

int xr_render(
    const xr_element* root,
    xr_command* destination,
    int capacity) {
    try {
        xaml::bridge::lastError.clear();
        if (root == nullptr) {
            throw std::invalid_argument("root is required");
        }
        xaml::bridge::RecordingBackend backend;
        xaml::Render(*reinterpret_cast<xaml::Element*>(const_cast<xr_element*>(root)), backend);
        const auto& commands = backend.Commands();
        if (destination != nullptr && capacity > 0) {
            const size_t count = std::min(commands.size(), static_cast<size_t>(capacity));
            std::copy_n(commands.data(), count, destination);
        }
        return static_cast<int>(commands.size());
    } catch (const std::exception& error) {
        xaml::bridge::lastError = error.what();
        return -1;
    }
}

int xr_render_angle(
    const xr_element* root,
    const char* fontPath,
    int width,
    int height,
    unsigned char* destination,
    int destinationStride,
    int destinationCapacity) {
    try {
        xaml::bridge::lastError.clear();
        if (root == nullptr || fontPath == nullptr || destination == nullptr
            || width <= 0 || height <= 0 || destinationStride < width * 4
            || destinationCapacity / destinationStride < height) {
            throw std::invalid_argument("Invalid ANGLE render arguments");
        }

        xaml::bridge::AngleRenderSurface surface(width, height, fontPath);
        surface.Render(
            *reinterpret_cast<xaml::Element*>(const_cast<xr_element*>(root)),
            destination,
            destinationStride);
        return 1;
    } catch (const std::exception& error) {
        xaml::bridge::lastError = error.what();
        return 0;
    }
}