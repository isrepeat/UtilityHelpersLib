#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace xaml {
    enum class XamlAttribute {
        id,
        text,
        fontSize,
        fontFamily,
        fontWeight,
        source,
        tint,
        command,
        foreground,
        orientation,
        verticalAlignment,
        horizontalAlignment,
        gridRow,
        gridColumn,
        rows,
        columns,
        background,
        activeBackground,
        borderBrush,
        activeBorderBrush,
        activeForeground,
        margin,
        padding,
        borderThickness,
        cornerRadius,
        width,
        height,
        isOn,
        visibility,
        isEnabled,
        opacity,
        itemsSource,
        count,
    };

    enum class XamlAttributeGroup : uint32_t {
        identity = 1 << 0,
        layout = 1 << 1,
        size = 1 << 2,
        gridPosition = 1 << 3,
        text = 1 << 4,
        source = 1 << 5,
        tint = 1 << 6,
        command = 1 << 7,
        orientation = 1 << 8,
        gridDefinitions = 1 << 9,
        background = 1 << 10,
        activeBackground = 1 << 16,
        border = 1 << 11,
        activeBorder = 1 << 17,
        activeForeground = 1 << 18,
        padding = 1 << 12,
        cornerRadius = 1 << 13,
        toggle = 1 << 14,
        itemsSource = 1 << 15,
    };

    std::optional<XamlAttribute> ParseXamlAttribute(std::string_view name);
    std::string_view XamlAttributeName(XamlAttribute attribute);
    XamlAttributeGroup XamlAttributeGroupOf(XamlAttribute attribute);
}