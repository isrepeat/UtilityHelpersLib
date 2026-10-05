#pragma once
#include "../../{{Application}}.Application/Core/ApplicationSession.h"
#include "./PreviewNavigationController.h"

namespace {{application}}::preview::session {
    class PreviewSession final {
    public:
        PreviewSession(int width, int height);
        xaml::Element& Root();
        PreviewNavigationController& Navigation();
        const {{application}}::application::core::PageManager& Pages() const;
        bool Navigate(std::span<const std::string_view> transitionIds, std::string& error);
        std::string_view CurrentPage() const;
        std::string_view PageTitle(std::string_view page) const;
        bool LoadPage(std::string_view page);
        void Resize(int width, int height);
        bool ReloadMarkup(std::string_view page, std::string_view markup, std::string_view sourcePath, std::string& error);
        void SetAnimationPlaybackRate(float value);
        void PointerDown(float x, float y);
        void PointerMove(float x, float y);
        void PointerUp(float x, float y);
        void CancelPointer();
        bool Update();
        void Render(xaml::IRenderBackend& renderer);
        static std::vector<std::string> ParseNavigationTransitionIds(std::string_view json);

    private:
        {{application}}::application::core::ApplicationSession applicationSession;
        PreviewNavigationController navigation;
    };
}