#include "XamlCompletionAbi.h"

#include <XamlRuntime/ElementBuilder.h>

#include "../Bridge/Diagnostic.h"

#include <string_view>
#include <stdexcept>
#include <iterator>
#include <vector>

namespace preview_sdk::abi {
    using namespace AndroidAppPreviewerPluginSDK;

    //
    // Методы
    //
    int XamlCompletionAbi::xp_supported_attribute_count(const char* elementType) {
        try {
            preview_sdk::bridge::LastError().clear();
            if (elementType == nullptr) {
                throw std::invalid_argument("element type is required");
            }
            if (std::string_view(elementType) == "columnDefinition" || std::string_view(elementType) == "rowDefinition") {
                return 1;
            }
            return static_cast<int>(xaml::SupportedAttributeNames(xaml::ParseElementType(elementType)).size());
        } catch (const std::exception& error) {
            preview_sdk::bridge::LastError() = error.what();
            return 0;
        }
    }

    const char* XamlCompletionAbi::xp_supported_attribute_name(
        const char* elementType,
        int index
    ) {
        try {
            preview_sdk::bridge::LastError().clear();
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
            preview_sdk::bridge::LastError() = error.what();
            return "";
        }
    }

    int XamlCompletionAbi::xp_supported_element_count(void) {
        return 18;
    }

    const char* XamlCompletionAbi::xp_supported_element_name(int index) {
        static constexpr std::string_view names[]{
            "Page", "StackPanel", "Grid", "Border", "TextBlock", "Button", "IconButton", "ToggleSwitch",
            "ScrollViewer", "Image", "SvgImage", "ListView", "ListView.ItemTemplate", "DataTemplate",
            "columnDefinitions", "columnDefinition", "rowDefinitions", "rowDefinition"
        };
        return index >= 0 && index < static_cast<int>(std::size(names)) ? names[index].data() : "";
    }
} // namespace preview_sdk::abi