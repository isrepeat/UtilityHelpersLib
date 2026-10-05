#pragma once
#include <XamlRuntime/RenderEngine.h>

#include "../UI/Page/SettingsPageViewModel.h"
#include "../UI/Page/MainPageViewModel.h"
#include "./InputDispatcher.h"
#include "./PageRegistry.h"
#include "./Navigation.h"

#include <vector>
#include <span>

namespace {{application}}::application::core {
    class PageManager final : public interface::IPageNavigator {
    public:
#if defined(ANDROID_APP_PREVIEWER)
        struct preview_Route final {
            std::string_view id;
            std::string_view source;
            std::string_view target;
            std::string_view backwardOfRouteId;
            std::string_view title;
            bool isDefault;
            NavigationTargetKind targetKind;
            std::string_view dataType;
            bool hasPreviewDefaultNavigationState;
        };
#endif

        PageManager(model::ApplicationRepository& applicationRepository, AppSessionController& appSessionController);
        ~PageManager() = default;
        PageManager(const PageManager&) = delete;
        PageManager& operator=(const PageManager&) = delete;

        //
        // IPageNavigator
        //
        bool Navigate(std::string_view pageName) override;
        bool Trigger(NavigationTrigger navigationTrigger) override;
        bool Trigger(
            NavigationTrigger navigationTrigger,
            std::unique_ptr<base::NavigationStateBase> navigationState) override;
        bool NavigateBack(std::unique_ptr<base::NavigationStateBase> navigationState = {}) override;

        void Initialize(xaml::Size size);
        void Resize(xaml::Size size);
        std::string_view CurrentPageName() const;
        std::string_view PageTitle(std::string_view name) const;
        xaml::Element& Root();
        bool IsTransitioning() const;
        void Render(xaml::IRenderBackend& renderer, const xaml::RendererRegistry& renderers);
        void PointerDown(float x, float y);
        void PointerMove(float x, float y);
        void PointerUp(float x, float y);
        void CancelPointer();
        bool Update();
        void SetAnimationPlaybackRate(float value);
#if defined(ANDROID_APP_PREVIEWER)
        std::vector<preview_Route> preview_Routes() const;
        bool preview_NavigateTransitions(std::span<const std::string_view> ids, std::string& error);
        bool preview_ReloadMarkup(std::string_view page, std::string_view markup, std::string_view sourcePath, std::string& error);
#endif

    private:
        struct NavigationRoute final {
            std::string_view id;
            std::string_view source;
            NavigationTrigger navigationTrigger;
            std::string_view target;
            NavigationTargetKind targetKind;
            const NavigationDataContract* dataContract;
            std::string_view title;
        };
        struct HistoryEntry final {
            interface::IPage* page;
            const NavigationRoute* incomingRoute;
        };
        static std::span<const NavigationRoute> Routes();
        std::string_view ResolveTarget(const NavigationRoute& navigationRoute) const;
        bool Navigate(
            const NavigationRoute& navigationRoute,
            std::unique_ptr<base::NavigationStateBase> navigationState);
        static bool IsNavigationDataValid(
            const NavigationRoute& navigationRoute,
            const base::NavigationStateBase* navigationState);
        void AttachAnimations();
        void UpdateTransition();

    private:
        PageContext pageContext;
        PageRegistry<ui::page::MainPageViewModel, ui::page::SettingsPageViewModel> pageRegistry;
        interface::IPage* currentPage = nullptr;
        interface::IPage* outgoingPage = nullptr;
        std::vector<HistoryEntry> history;
        xaml::Size viewport{};
        xaml::AnimationController animations;
        InputDispatcher input;
        float playbackRate = 1.0f;
        bool navigating = false;
        bool dirty = true;
    };
}