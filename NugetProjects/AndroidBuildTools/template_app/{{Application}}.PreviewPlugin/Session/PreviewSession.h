#pragma once
#include <XamlRuntime/InteractionController.h>
#include <XamlRuntime/XamlLayout.h>
#include <XamlRuntime/Animation.h>
#include <XamlRuntime/Binding.h>

#include "./PreviewNavigationController.h"

#include <string_view>
#include <memory>
#include <string>
#include <vector>

namespace {{application}}::preview::session {
    class PreviewSession final {
    public:
        PreviewSession(int width, int height);

        xaml::Element& Root();
        PreviewNavigationController& Navigation();
        std::string_view CurrentPage() const;
        std::string_view PageTitle(std::string_view page) const;
        bool LoadPage(std::string_view page);
        void QueuePage(std::string_view page);
        void Resize(int width, int height);
        bool ReloadMarkup(std::string_view page, std::string_view markup, std::string_view sourcePath, std::string& error);
        void SetAnimationPlaybackRate(float value);
        void PointerDown(float x, float y);
        void PointerMove(float x, float y);
        void PointerUp(float x, float y);
        void CancelPointer();
        bool Update();
        static std::vector<std::string> ParseNavigationTransitionIds(std::string_view json);

    private:
        struct ViewModel final {
            PreviewSession* session = nullptr;

            xaml::Element::Command NavigateToMainCommand();
            xaml::Element::Command NavigateToSettingsCommand();
            xaml::Element::Command UpdateApplicationCommand();
        };

        bool IsKnownPage(std::string_view page) const;

    private:
        ViewModel viewModel;
        std::unique_ptr<xaml::Element> root;
        std::unique_ptr<xaml::BindingScope> bindings;
        xaml::AnimationController animations;
        xaml::InteractionController interaction;
        PreviewNavigationController navigation;
        xaml::Size viewport{};
        std::string currentPage;
        std::string pendingPage;
    };
}