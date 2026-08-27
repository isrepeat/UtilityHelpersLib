#pragma once
#include "Components.g.h"

namespace winrt::CppFeatures::WinRt::implementation
{
    struct Components : ComponentsT<Components>
    {
        Components() = default;
    };
}
namespace winrt::CppFeatures::WinRt::factory_implementation
{
    struct Components : ComponentsT<Components, implementation::Components>
    {
    };
}