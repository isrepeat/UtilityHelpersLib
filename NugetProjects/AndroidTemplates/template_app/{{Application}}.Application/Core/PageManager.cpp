#include "PageManager.h"

#if defined(ANDROID_APP_PREVIEWER)
#include <XamlRuntime/RuntimeMarkup/XamlParser.h>
#endif

#include "./NavigationStates.h"

#include <algorithm>
#include <stdexcept>
#include <array>

namespace {{application}}::application::core {
    PageManager::PageManager(model::ApplicationRepository& repository)
        : context{*this, repository}
        , pages(this->context) {
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
        for (const auto& route : Routes()) {
            if (route.source == this->CurrentPageName() && this->ResolveTarget(route) == pageName) {
                return this->Navigate(route, {});
            }
        }
        return false;
    }

    bool PageManager::Trigger(NavigationTrigger trigger) {
        return this->Trigger(trigger, {});
    }

    bool PageManager::Trigger(NavigationTrigger trigger, std::unique_ptr<base::NavigationStateBase> state) {
        for (const auto& route : Routes()) {
            if (route.source == this->CurrentPageName() && route.trigger == trigger) {
                return this->Navigate(route, std::move(state));
            }
        }
        return false;
    }

    bool PageManager::NavigateBack(std::unique_ptr<base::NavigationStateBase> result) {
        return this->Trigger(NavigationTrigger::navigateBack, std::move(result));
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
        this->pages.ForEach([size](interface::IPage& page) { page.Initialize(size); });
        this->currentPage = this->pages.Find(ui::page::MainPageViewModel::PageName);
        const NavigationRequest request{{}, this->currentPage->Name(), NavigationTrigger::navigateBack};
        if (!this->currentPage->OnNavigatingTo(request, {})) {
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
        this->pages.ForEach([size](interface::IPage& page) { xaml::layoutInViewport(page.Root(), size); });
        this->dirty = true;
    }

    std::string_view PageManager::CurrentPageName() const {
        return this->currentPage == nullptr ? std::string_view{} : this->currentPage->Name();
    }

    std::string_view PageManager::PageTitle(std::string_view name) const {
        const auto* page = this->pages.Find(name);
        return page == nullptr ? std::string_view{} : page->Title();
    }

    xaml::Element& PageManager::Root() {
        if (this->currentPage == nullptr) {
            throw std::logic_error("Application session is not initialized");
        }
        return this->currentPage->Root();
    }

    void PageManager::PointerDown(float x, float y) {
        this->input.PointerDown(this->Root(), this->animations, x, y);
    }

    void PageManager::PointerMove(float x, float y) {
        this->input.PointerMove(x, y);
    }

    void PageManager::PointerUp(float x, float y) {
        this->input.PointerUp(this->Root(), this->animations, x, y);
    }

    void PageManager::CancelPointer() {
        this->input.Cancel();
    }

    bool PageManager::Update() {
        const bool interacting = this->input.Update();
        const bool animating = this->animations.IsAnimating();
        this->animations.Update();
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
#if defined(ANDROID_APP_PREVIEWER)
    std::vector<PageManager::preview_Route> PageManager::preview_Routes() const {
        std::vector<preview_Route> result;
        for (const auto& route : Routes()) {
            if (route.targetKind == NavigationTargetKind::previousPage) {
                // Граф показывает обратные рёбра для входящих маршрутов. Выполнение
                // всегда использует фактическую историю, а не цель из графа.
                for (const auto& incoming : Routes()) {
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
        if (ids.empty()) {
            error = "Navigation requires transition IDs";
            return false;
        }
        // Проверяем весь путь до первого изменения страниц и истории.
        std::vector<std::string_view> simulated;
        for (const auto& entry : this->history) {
            simulated.push_back(entry.page->Name());
        }
        std::vector<const NavigationRoute*> routes;
        std::vector<std::unique_ptr<base::NavigationStateBase>> states;
        for (const auto id : ids) {
            const auto available = Routes();
            const auto found = std::find_if(available.begin(), available.end(), [id](const auto& route) { return route.id == id; });
            if (found == available.end() || simulated.empty() || found->source != simulated.back()) {
                error = "Transition is not available on the current page";
                return false;
            }
            std::unique_ptr<base::NavigationStateBase> state;
            if (found->dataContract != nullptr && found->dataContract->preview_CreatePreviewDefaultFn != nullptr) {
                state = found->dataContract->preview_CreatePreviewDefaultFn();
            }
            if (!IsNavigationDataValid(*found, state.get())) {
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
            states.push_back(std::move(state));
        }
        for (size_t i = 0; i < routes.size(); ++i) {
            if (!this->Navigate(*routes[i], std::move(states[i]))) {
                error = "Page lifecycle rejected navigation";
                return false;
            }
        }
        error.clear();
        return true;
    }

    bool PageManager::preview_ReloadMarkup(std::string_view page, std::string_view markup, std::string_view sourcePath, std::string& error) {
        auto* target = this->pages.Find(page);
        if (target == nullptr) {
            error = "Unknown page";
            return false;
        }
        auto tree = xaml::runtime::RuntimeTreeBuilder{}.BuildPage(
            xaml::runtime::XamlParser{}.Parse(markup, sourcePath), target->preview_RuntimeContext(), this->viewport);
        this->input.Cancel();
        target->preview_ReplaceRuntimeTree(std::move(tree));
        if (target == this->currentPage) {
            this->AttachAnimations();
        }
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

    std::string_view PageManager::ResolveTarget(const NavigationRoute& route) const {
        if (route.targetKind == NavigationTargetKind::page) {
            return route.target;
        }
        return this->history.size() < 2 ? std::string_view{} : this->history[this->history.size() - 2].page->Name();
    }

    bool PageManager::Navigate(const NavigationRoute& route, std::unique_ptr<base::NavigationStateBase> state) {
        auto* target = this->pages.Find(this->ResolveTarget(route));
        if (this->navigating || target == nullptr || this->currentPage == nullptr || route.source != this->CurrentPageName()) {
            return false;
        }
        this->navigating = true;
        struct NavigationGuard final {
            bool& active;
            ~NavigationGuard() { active = false; }
        } guard{this->navigating};
        const NavigationRequest request{route.source, target->Name(), route.trigger};
        auto outgoingState = this->currentPage->OnNavigatingFrom(request);
        if (!state) {
            state = std::move(outgoingState);
        }
        if (!IsNavigationDataValid(route, state.get()) || !target->OnNavigatingTo(request, std::move(state))) {
            return false;
        }
        this->input.Cancel();
        this->currentPage = target;
        if (route.targetKind == NavigationTargetKind::previousPage) {
            this->history.pop_back();
        } else {
            this->history.push_back({target, &route});
        }
        this->AttachAnimations();
        xaml::layoutInViewport(this->Root(), this->viewport);
        this->dirty = true;
        return true;
    }

    bool PageManager::IsNavigationDataValid(const NavigationRoute& route, const base::NavigationStateBase* state) {
        if (route.dataContract == nullptr) {
            return state == nullptr;
        }
        return state == nullptr ? !route.dataContract->isRequired : state->TypeId() == route.dataContract->typeId;
    }

    void PageManager::AttachAnimations() {
        this->animations = xaml::AnimationController{};
        this->animations.Attach(this->Root(), xaml::AnimationRegistry{}, false);
        this->animations.SetPlaybackRate(this->playbackRate);
    }
}