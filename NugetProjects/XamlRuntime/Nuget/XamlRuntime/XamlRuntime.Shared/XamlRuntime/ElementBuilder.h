#pragma once

#include "XamlRuntime/XamlLayout.h"

#include <string_view>

namespace xaml {
    ElementType ParseElementType(std::string_view name);
    void SetAttribute(Element& element, std::string_view name, std::string_view value);
}