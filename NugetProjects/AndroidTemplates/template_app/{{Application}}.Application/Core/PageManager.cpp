#include "PageManager.h"

#if defined(ANDROID_APP_PREVIEWER)
#include <XamlRuntime/RuntimeMarkup/XamlParser.h>
#endif

#include "./NavigationStates.h"

#include <algorithm>
#include <stdexcept>
#include <chrono>
#include <limits>
#include <array>

namespace {{application}}::application::core {
    PageManager::PageManager(model::ApplicationRepository& applicationRepository, AppSessionController& appSessionController)
        : pageContext{*this, applicationRepository, appSessionController, appSessionController}
        , pageRegistry(this->pageContext) {
    }

    //
    // IPageNavigator
    //
    bool PageManager::Navigate(std::string_view pageName) {
        if (this->currentPage == nullptr) {
            return false;
        }
        if (pageName == this->CurrentPageName()) {
            return true;
        }
        for (const auto& route : this->Routes()) {
            if (route.source == this->CurrentPageName() && this->ResolveTarget(route) == pageName) {
                return this->Navigate(route, {});
            }
        }
        return false;
    }

    bool PageManager::Trigger(NavigationTrigger navigationTrigger) {
        return this->Trigger(navigationTrigger, {});
    }

    bool PageManager::Trigger(
        NavigationTrigger navigationTrigger,
        std::unique_ptr<base::NavigationStateBase> navigationState) {
        for (const auto& route : this->Routes()) {
            if (route.source == this->CurrentPageName() && route.navigationTrigger == navigationTrigger) {
                return this->Navigate(route, std::move(navigationState));
            }
        }
        return false;
    }

    bool PageManager::NavigateBack(std::unique_ptr<base::NavigationStateBase> navigationState) {
        return this->Trigger(NavigationTrigger::navigateBack, std::move(navigationState));
    }

    //
    // API
    //
    void PageManager::Initialize(xaml::Size size) {
        if (size.width <= 0 || size.height <= 0) {
            throw std::invalid_argument("Viewport dimensions must be positive");
        }
        if (this->currentPage != nullptr) {
            this->Resize(size);
            return;
        }
        this->viewport = size;
        this->pageRegistry.ForEach([size](interface::IPage& page) { page.Initialize(size); });
        this->currentPage = this->pageRegistry.Find(ui::page::MainPageViewModel::PageName);
        const NavigationRequest navigationRequest{{}, this->currentPage->Name(), NavigationTrigger::navigateBack};
        if (!this->currentPage->OnNavigatingTo(navigationRequest, {})) {
            throw std::runtime_error("Initial page rejected activation");
        }
        this->history.push_back({this->currentPage, nullptr});
        this->AttachAnimations();
    }

    void PageManager::Resize(xaml::Size size) {
        if (size.width <= 0 || size.height <= 0) {
            throw std::invalid_argument("Viewport dimensions must be positive");
        }
        this->viewport = size;
        this->pageRegistry.ForEach([size](interface::IPage& page) { xaml::layoutInViewport(page.Root(), size); });
        this->dirty = true;
    }

    std::string_view PageManager::CurrentPageName() const {
        return this->currentPage == nullptr ? std::string_view{} : this->currentPage->Name();
    }

    std::string_view PageManager::PageTitle(std::string_view name) const {
        const auto* page = this->pageRegistry.Find(name);
        return page == nullptr ? std::string_view{} : page->Title();
    }

    xaml::Element& PageManager::Root() {
        if (this->currentPage == nullptr) {
            throw std::logic_error("Application session is not initialized");
        }
        return this->currentPage->Root();
    }

    void PageManager::PointerDown(float x, float y) {
        if (this->IsTransitioning()) {
            return;
        }
        this->input.PointerDown(this->Root(), this->animations, x, y);
    }

    void PageManager::PointerMove(float x, float y) {
        if (this->IsTransitioning()) {
            return;
        }
        this->input.PointerMove(x, y);
    }

    void PageManager::PointerUp(float x, float y) {
        if (this->IsTransitioning()) {
            return;
        }
        this->input.PointerUp(this->Root(), this->animations, x, y);
    }

    void PageManager::CancelPointer() {
        this->input.Cancel();
    }

    bool PageManager::Update() {
        const bool interacting = this->input.Update();
        const bool animating = this->animations.IsAnimating();
        this->animations.Update();
        this->UpdateTransition();
        if (this->outgoingPage != nullptr) {
            this->outgoingPage->Update();
            xaml::layoutInViewport(this->outgoingPage->Root(), this->viewport);
        }
        this->currentPage->Update();
        xaml::layoutInViewport(this->Root(), this->viewport);
        const bool changed = this->dirty || interacting || animating || this->animations.IsAnimating();
        this->dirty = false;
        return changed;
    }

    void PageManager::SetAnimationPlaybackRate(float value) {
        this->playbackRate = value;
        this->animations.SetPlaybackRate(value);
    }

    bool PageManager::IsTransitioning() const {
        return this->outgoingPage != nullptr;
    }

    void PageManager::Render(xaml::IRenderBackend& renderer, const xaml::RendererRegistry& renderers) {
        if (this->outgoingPage != nullptr) {
            xaml::Render(this->outgoingPage->Root(), renderer, renderers);
        }
        xaml::Render(this->Root(), renderer, renderers);
    }

#if defined(ANDROID_APP_PREVIEWER)
    std::vector<PageManager::preview_Route> PageManager::preview_Routes() const {
        std::vector<preview_Route> result;
        for (const auto& route : this->Routes()) {
            if (route.targetKind == NavigationTargetKind::previousPage) {
                // Граф показывает обратные рёбра для входящих маршрутов. Выполнение
                // всегда использует фактическую историю, а не цель из графа.
                for (const auto& incoming : this->Routes()) {
                    if (incoming.targetKind == NavigationTargetKind::page && incoming.target == route.source) {
                        result.push_back({route.id, route.source, incoming.source, incoming.id, route.title,
                            true, route.targetKind, {}, true});
                    }
                }
            } else {
                const auto* contract = route.dataContract;
                result.push_back({route.id, route.source, route.target, {}, route.title, true, route.targetKind,
                    contract == nullptr ? std::string_view{} : contract->typeId,
                    contract == nullptr || !contract->isRequired || contract->preview_CreatePreviewDefaultFn != nullptr});
            }
        }
        return result;
    }

    bool PageManager::preview_NavigateTransitions(std::span<const std::string_view> ids, std::string& error) {
        if (ids.empty() || this->IsTransitioning()) {
            error = "Navigation requires transition IDs and an idle page";
            return false;
        }
        // Проверяем весь путь до первого изменения страниц и истории.
        std::vector<std::string_view> simulated;
        for (const auto& entry : this->history) {
            simulated.push_back(entry.page->Name());
        }
        std::vector<const NavigationRoute*> routes;
        std::vector<std::unique_ptr<base::NavigationStateBase>> navigationStates;
        for (const auto id : ids) {
            const auto available = this->Routes();
            const auto found = std::find_if(available.begin(), available.end(), [id](const auto& route) { return route.id == id; });
            if (found == available.end() || simulated.empty() || found->source != simulated.back()) {
                error = "Transition is not available on the current page";
                return false;
            }
            std::unique_ptr<base::NavigationStateBase> navigationState;
            if (found->dataContract != nullptr && found->dataContract->preview_CreatePreviewDefaultFn != nullptr) {
                navigationState = found->dataContract->preview_CreatePreviewDefaultFn();
            }
            if (!this->IsNavigationDataValid(*found, navigationState.get())) {
                error = "Native preview default does not satisfy the data contract";
                return false;
            }
            if (found->targetKind == NavigationTargetKind::previousPage) {
                if (simulated.size() < 2) {
                    error = "Navigation history has no previous page";
                    return false;
                }
                simulated.pop_back();
            } else {
                simulated.push_back(found->target);
            }
            routes.push_back(&*found);
            navigationStates.push_back(std::move(navigationState));
        }
        for (size_t i = 0; i < routes.size(); ++i) {
            if (!this->Navigate(*routes[i], std::move(navigationStates[i]))) {
                error = "Page lifecycle rejected navigation";
                return false;
            }
            if (i + 1 < routes.size() && this->IsTransitioning()) {
                // Промежуточные страницы пути графа доводим до конечного состояния;
                // последний переход проигрывается обычным циклом кадров.
                const auto elapsed = std::chrono::duration<float, std::milli>(std::numeric_limits<float>::max());
                xaml::AnimationController::Update(this->outgoingPage->Root(), elapsed);
                xaml::AnimationController::Update(this->Root(), elapsed);
                this->UpdateTransition();
            }
        }
        error.clear();
        return true;
    }

    bool PageManager::preview_ReloadMarkup(std::string_view page, std::string_view markup, std::string_view sourcePath, std::string& error) {
        auto* target = this->pageRegistry.Find(page);
        if (target == nullptr) {
            error = "Unknown page";
            return false;
        }
        auto tree = xaml::runtime::RuntimeTreeBuilder{}.BuildPage(
            xaml::runtime::XamlParser{}.Parse(markup, sourcePath), target->preview_RuntimeContext(), this->viewport);
        this->input.Cancel();
        target->preview_ReplaceRuntimeTree(std::move(tree));
        // Новый runtime-корень тоже подключается к общему контроллеру.
        this->AttachAnimations();
        this->dirty = true;
        error.clear();
        return true;
    }
#endif
    //
    // Internal
    //
    std::span<const PageManager::NavigationRoute> PageManager::Routes() {
        static const std::array routes{
            NavigationRoute{"main-to-settings", "MainPage", NavigationTrigger::navigateToSettings, "SettingsPage",
                NavigationTargetKind::page, &GreetingNavigationState::Contract(), "Open settings"},
            NavigationRoute{"settings-to-main", "SettingsPage", NavigationTrigger::navigateBack, {},
                NavigationTargetKind::previousPage, nullptr, "Back"},
        };
        return routes;
    }

    std::string_view PageManager::ResolveTarget(const NavigationRoute& navigationRoute) const {
        if (navigationRoute.targetKind == NavigationTargetKind::page) {
            return navigationRoute.target;
        }
        return this->history.size() < 2 ? std::string_view{} : this->history[this->history.size() - 2].page->Name();
    }

    bool PageManager::Navigate(
        const NavigationRoute& navigationRoute,
        std::unique_ptr<base::NavigationStateBase> navigationState) {
        auto* target = this->pageRegistry.Find(this->ResolveTarget(navigationRoute));
        if (this->navigating || this->IsTransitioning() || target == nullptr || target == this->currentPage
            || this->currentPage == nullptr || navigationRoute.source != this->CurrentPageName()) {
            return false;
        }
        this->navigating = true;
        struct NavigationGuard final {
            bool& active;
            ~NavigationGuard() { active = false; }
        } guard{this->navigating};
        const NavigationRequest navigationRequest{
            navigationRoute.source,
            target->Name(),
            navigationRoute.navigationTrigger};
        auto outgoingNavigationState = this->currentPage->OnNavigatingFrom(navigationRequest);
        if (!navigationState) {
            navigationState = std::move(outgoingNavigationState);
        }
        if (!this->IsNavigationDataValid(navigationRoute, navigationState.get())
            || !target->OnNavigatingTo(navigationRequest, std::move(navigationState))) {
            return false;
        }
        this->input.Cancel();
        this->outgoingPage = this->currentPage;
        this->currentPage = target;
        if (navigationRoute.targetKind == NavigationTargetKind::previousPage) {
            this->history.pop_back();
        } else {
            this->history.push_back({target, &navigationRoute});
        }
        this->outgoingPage->Root().SetVisibility(xaml::attr::Visibility::collapsed);
        this->Root().SetVisibility(xaml::attr::Visibility::visible);
        const std::string direction = navigationRoute.targetKind == NavigationTargetKind::previousPage ? "Backward" : "Forward";
        for (auto* page : {this->outgoingPage, this->currentPage}) {
            xaml::VisualStateManager::GoToState(page->Root(), "NavigationDirection", "Idle", false);
            xaml::VisualStateManager::GoToState(page->Root(), "NavigationDirection", direction);
            xaml::layoutInViewport(page->Root(), this->viewport);
        }
        this->UpdateTransition();
        this->dirty = true;
        return true;
    }

    bool PageManager::IsNavigationDataValid(
        const NavigationRoute& navigationRoute,
        const base::NavigationStateBase* navigationState) {
        if (navigationRoute.dataContract == nullptr) {
            return navigationState == nullptr;
        }
        return navigationState == nullptr
            ? !navigationRoute.dataContract->isRequired
            : navigationState->TypeId() == navigationRoute.dataContract->typeId;
    }

    void PageManager::AttachAnimations() {
        this->animations = xaml::AnimationController{};
        this->outgoingPage = nullptr;
        this->pageRegistry.ForEach([this](interface::IPage& page) {
            // Видимость устанавливаем до Attach, без проигрывания перехода при загрузке.
            page.Root().SetVisibility(&page == this->currentPage
                ? xaml::attr::Visibility::visible : xaml::attr::Visibility::collapsed);
            this->animations.Attach(page.Root(), xaml::AnimationRegistry{});
        });
        this->animations.SetPlaybackRate(this->playbackRate);
    }

    void PageManager::UpdateTransition() {
        if (this->outgoingPage != nullptr
            && !xaml::AnimationController::IsAnimating(this->outgoingPage->Root())
            && !xaml::AnimationController::IsAnimating(this->Root())) {
            this->outgoingPage = nullptr;
        }
    }
}