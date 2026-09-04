#include <Helpers.Logging/Logging.h>

#include <XamlRuntime/ElementBuilder.h>
#include <XamlRuntime/RenderEngine.h>
#include <XamlRuntime/XamlLayout.h>
#include <XamlRuntime/Animation.h>
#include <XamlRuntime/Input.h>

#include "AngleRenderSurface.h"
#include "NativeBridge.h"

#include <string_view>
#include <filesystem>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <cmath>

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
            std::string_view fontWeight,
            attr::Alignment) override {
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

namespace xaml::bridge::_details {
    // TODO(TEMP_REMOVE): удалить после отладки геометрии анимации кнопки.
    void LogButtonGeometry(
        const Element& element,
        Rect bounds,
        std::string_view phase) {
        constexpr int SegmentsPerCorner = 8;
        constexpr float Pi = 3.14159265358979323846f;
        const float maximumRadius = std::max(0.0f, std::min(bounds.width, bounds.height) / 2.0f);
        const float radius = std::clamp(element.CornerRadius(), 0.0f, maximumRadius);
        const float centers[][2] = {
            {bounds.x + bounds.width - radius, bounds.y + radius},
            {bounds.x + bounds.width - radius, bounds.y + bounds.height - radius},
            {bounds.x + radius, bounds.y + bounds.height - radius},
            {bounds.x + radius, bounds.y + radius},
        };

        LOG_DEBUG(
            "XamlPreviewer.NativeBridge",
            "TEMP_REMOVE button geometry: phase={}, id='{}', bounds=({}, {}, {}, {}), radius={}",
            phase,
            element.Id(),
            bounds.x,
            bounds.y,
            bounds.width,
            bounds.height,
            radius);
        int pointIndex = 0;
        for (int corner = 0; corner < 4; ++corner) {
            const float startAngle = -Pi / 2.0f + static_cast<float>(corner) * Pi / 2.0f;
            for (int segment = 0; segment <= SegmentsPerCorner; ++segment) {
                const float angle = startAngle
                    + static_cast<float>(segment) * Pi / (2.0f * SegmentsPerCorner);
                float cosine = std::cos(angle);
                float sine = std::sin(angle);
                if (std::abs(cosine) < 0.000001f) {
                    cosine = 0.0f;
                }
                if (std::abs(sine) < 0.000001f) {
                    sine = 0.0f;
                }
                if (std::abs(std::abs(cosine) - 1.0f) < 0.000001f) {
                    cosine = cosine < 0.0f ? -1.0f : 1.0f;
                }
                if (std::abs(std::abs(sine) - 1.0f) < 0.000001f) {
                    sine = sine < 0.0f ? -1.0f : 1.0f;
                }
                LOG_DEBUG(
                    "XamlPreviewer.NativeBridge",
                    "TEMP_REMOVE button geometry: phase={}, point={}, x={}, y={}",
                    phase,
                    pointIndex++,
                    centers[corner][0] + cosine * radius,
                    centers[corner][1] + sine * radius);
            }
        }
        LOG_DEBUG(
            "XamlPreviewer.NativeBridge",
            "TEMP_REMOVE button geometry: phase={}, point={}, x={}, y={} (closing point)",
            phase,
            pointIndex,
            centers[0][0],
            bounds.y);
    }

    Rect ScaleForPressedButton(Rect bounds) {
        constexpr float PressedScale = 0.94f;
        const float width = bounds.width * PressedScale;
        const float height = bounds.height * PressedScale;
        return {
            bounds.x + (bounds.width - width) / 2.0f,
            bounds.y + (bounds.height - height) / 2.0f,
            width,
            height,
        };
    }
}

struct xr_animation_controller {
    xaml::AnimationController value;
};

struct xr_angle_surface {
    explicit xr_angle_surface(
        int width,
        int height,
        const char* fontPath,
        const char* resourceRoot)
        : width(width)
        , height(height)
        , value(width, height, fontPath, resourceRoot) {
    }

    int width;
    int height;
    xaml::bridge::AngleRenderSurface value;
};

const char* xr_last_error(void) {
    return xaml::bridge::lastError.c_str();
}

void xr_configure_logging(const char* filePath) {
    utility_helpers::logging::Configure({
        filePath == nullptr ? std::filesystem::path{} : std::filesystem::path(filePath),
    });
    utility_helpers::logging::Initialize("XamlPreviewer");
    LOG_INFO("XamlPreviewer.NativeBridge", "Logging initialized");
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

int xr_supported_attribute_count(const char* elementType) {
    try {
        xaml::bridge::lastError.clear();
        if (elementType == nullptr) {
            throw std::invalid_argument("element type is required");
        }
        if (std::string_view(elementType) == "columnDefinition" || std::string_view(elementType) == "rowDefinition") {
            return 1;
        }
        return static_cast<int>(xaml::SupportedAttributeNames(xaml::ParseElementType(elementType)).size());
    } catch (const std::exception& error) {
        xaml::bridge::lastError = error.what();
        return 0;
    }
}

const char* xr_supported_attribute_name(
    const char* elementType,
    int index) {
    try {
        xaml::bridge::lastError.clear();
        if (elementType == nullptr || index < 0) {
            throw std::invalid_argument("element type and non-negative index are required");
        }
        if (std::string_view(elementType) == "columnDefinition" && index == 0) {
            return "width";
        }
        if (std::string_view(elementType) == "rowDefinition" && index == 0) {
            return "height";
        }
        const std::vector<std::string_view> names = xaml::SupportedAttributeNames(
            xaml::ParseElementType(elementType));
        if (index >= static_cast<int>(names.size())) {
            throw std::out_of_range("attribute index is out of range");
        }
        return names[static_cast<size_t>(index)].data();
    } catch (const std::exception& error) {
        xaml::bridge::lastError = error.what();
        return "";
    }
}

int xr_supported_element_count(void) {
    return 18;
}

const char* xr_supported_element_name(int index) {
    static constexpr std::string_view names[]{
        "Page", "StackPanel", "Grid", "Border", "TextBlock", "Button", "IconButton", "ToggleSwitch",
        "ScrollViewer", "Image", "SvgImage", "ListView", "ListView.ItemTemplate", "DataTemplate",
        "columnDefinitions", "columnDefinition", "rowDefinitions", "rowDefinition"
    };
    return index >= 0 && index < static_cast<int>(std::size(names)) ? names[index].data() : "";
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

xr_element* xr_hit_test(xr_element* root, float x, float y) {
    try {
        xaml::bridge::lastError.clear();
        if (root == nullptr) {
            throw std::invalid_argument("root is required");
        }
        return reinterpret_cast<xr_element*>(xaml::HitTest(
            *reinterpret_cast<xaml::Element*>(root), x, y));
    } catch (const std::exception& error) {
        xaml::bridge::lastError = error.what();
        return nullptr;
    }
}

const char* xr_element_id(const xr_element* element) {
    if (element == nullptr) {
        return "";
    }
    return reinterpret_cast<const xaml::Element*>(element)->Id().c_str();
}

int xr_handle_tap(xr_element* element, xr_animation_controller* animations) {
    try {
        xaml::bridge::lastError.clear();
        if (element == nullptr || animations == nullptr) {
            throw std::invalid_argument("element and animations are required");
        }
        xaml::Element& target = *reinterpret_cast<xaml::Element*>(element);
        const bool wasOn = target.IsOn();
        if (!xaml::HandleTap(target)) {
            return 0;
        }
        if (target.Type() == xaml::ElementType::toggleSwitch) {
            animations->value.Animate(
                target,
                xaml::AnimatedProperty::toggleProgress,
                wasOn ? 1.0f : 0.0f,
                wasOn ? 0.0f : 1.0f,
                std::chrono::milliseconds(120));
        }
        return 1;
    } catch (const std::exception& error) {
        xaml::bridge::lastError = error.what();
        return 0;
    }
}

int xr_handle_pointer_down(xr_element* element, xr_animation_controller* animations) {
    try {
        xaml::bridge::lastError.clear();
        if (element == nullptr || animations == nullptr) {
            throw std::invalid_argument("element and animations are required");
        }
        xaml::Element& target = *reinterpret_cast<xaml::Element*>(element);
        if (target.Type() != xaml::ElementType::button) {
            return 1;
        }
        // TODO(TEMP_REMOVE): журналируем обе крайние геометрии до исправления rounded-rect.
        xaml::bridge::_details::LogButtonGeometry(
            target,
            target.Bounds(),
            "before-press-animation");
        xaml::bridge::_details::LogButtonGeometry(
            target,
            xaml::bridge::_details::ScaleForPressedButton(target.Bounds()),
            "press-animation-end");
        animations->value.Animate(
            target,
            xaml::AnimatedProperty::pressProgress,
            target.PressProgress(),
            1.0f,
            std::chrono::milliseconds(80));
        return 1;
    } catch (const std::exception& error) {
        xaml::bridge::lastError = error.what();
        return 0;
    }
}

int xr_handle_pointer_up(xr_element* element, xr_animation_controller* animations) {
    try {
        xaml::bridge::lastError.clear();
        if (element == nullptr || animations == nullptr) {
            throw std::invalid_argument("element and animations are required");
        }
        xaml::Element& target = *reinterpret_cast<xaml::Element*>(element);
        if (target.Type() == xaml::ElementType::button) {
            animations->value.Animate(
                target,
                xaml::AnimatedProperty::pressProgress,
                target.PressProgress(),
                0.0f,
                std::chrono::milliseconds(150));
            return 1;
        }
        return xr_handle_tap(element, animations);
    } catch (const std::exception& error) {
        xaml::bridge::lastError = error.what();
        return 0;
    }
}

xr_animation_controller* xr_create_animation_controller(void) {
    try {
        xaml::bridge::lastError.clear();
        return new xr_animation_controller();
    } catch (const std::exception& error) {
        xaml::bridge::lastError = error.what();
        return nullptr;
    }
}

void xr_destroy_animation_controller(xr_animation_controller* animations) {
    delete animations;
}

int xr_update_animations(xr_animation_controller* animations) {
    try {
        xaml::bridge::lastError.clear();
        if (animations == nullptr) {
            throw std::invalid_argument("animations are required");
        }
        animations->value.Update();
        return animations->value.IsAnimating() ? 1 : 0;
    } catch (const std::exception& error) {
        xaml::bridge::lastError = error.what();
        return -1;
    }
}

xr_angle_surface* xr_create_angle_surface(
    int width,
    int height,
    const char* fontPath,
    const char* resourceRoot) {
    try {
        xaml::bridge::lastError.clear();
        if (fontPath == nullptr || resourceRoot == nullptr) {
            throw std::invalid_argument("fontPath and resourceRoot are required");
        }
        return new xr_angle_surface(width, height, fontPath, resourceRoot);
    } catch (const std::exception& error) {
        xaml::bridge::lastError = error.what();
        return nullptr;
    }
}

void xr_destroy_angle_surface(xr_angle_surface* surface) {
    delete surface;
}

int xr_render_angle_surface(
    xr_angle_surface* surface,
    const xr_element* root,
    unsigned char* destination,
    int destinationStride,
    int destinationCapacity) {
    try {
        xaml::bridge::lastError.clear();
        if (surface == nullptr || root == nullptr || destination == nullptr
            || destinationStride < surface->width * 4
            || destinationCapacity / destinationStride < surface->height) {
            throw std::invalid_argument("Invalid persistent ANGLE render arguments");
        }
        surface->value.Render(
            *reinterpret_cast<xaml::Element*>(const_cast<xr_element*>(root)),
            destination,
            destinationStride);
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
    const char* resourceRoot,
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

        xaml::bridge::AngleRenderSurface surface(width, height, fontPath, resourceRoot);
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