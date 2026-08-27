#pragma once
#include "Components.g.h"

namespace winrt::CrashHandling::WinRt::implementation
{
    struct Components : ComponentsT<Components>
    {
        Components() = default;
    };
}
namespace winrt::CrashHandling::WinRt::factory_implementation
{
    struct Components : ComponentsT<Components, implementation::Components>
    {
    };
}