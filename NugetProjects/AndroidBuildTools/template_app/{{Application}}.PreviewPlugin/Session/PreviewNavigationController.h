#pragma once
#include <string_view>
#include <string>
#include <span>

namespace {{application}}::preview::session {
    class PreviewNavigationController final {
    public:
        std::string BuildGraphJson() const;
        bool Navigate(std::span<const std::string_view> transitionIds, std::string& error) const;
    };
}