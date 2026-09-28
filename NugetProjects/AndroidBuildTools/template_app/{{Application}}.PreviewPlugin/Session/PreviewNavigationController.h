#pragma once
#include <string_view>
#include <string>
#include <span>

namespace {{application}}::preview::session {
    class PreviewSession;

    class PreviewNavigationController final {
    public:
        explicit PreviewNavigationController(PreviewSession& session);

        std::string BuildGraphJson() const;
        bool Navigate(std::span<const std::string_view> transitionIds, std::string& error);

    private:
        PreviewSession& session;
    };
}