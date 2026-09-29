#include "MainPageViewModel.h"

#if defined(ANDROID_APP_PREVIEWER)
#include <XamlRuntime/RuntimeMarkup/RuntimeBindingRegistry.h>
#endif

#include "../../Core/NavigationStates.h"
#include "../../../!Generated/{{Application}}.Application/Xaml/Page/MainPage.xaml.h"

#include <utility>

namespace {{application}}::application::ui::page {
    MainPageViewModel::MainPageViewModel(core::PageContext& context)
        : context(context) {
    }

    //
    // INavigationPage
    //
    std::unique_ptr<base::NavigationStateBase> MainPageViewModel::OnNavigatingFrom(const core::NavigationRequest&) {
        return {};
    }

    bool MainPageViewModel::OnNavigatingTo(const core::NavigationRequest&, std::unique_ptr<base::NavigationStateBase> state) {
        return state == nullptr;
    }

    //
    // IPage
    //
    std::string_view MainPageViewModel::Name() const {
        return PageName;
    }

    std::string_view MainPageViewModel::Title() const {
        return "Main";
    }

    void MainPageViewModel::Initialize(xaml::Size availableSize) {
        this->bindings = std::make_unique<xaml::BindingScope>();
        this->root = xaml::generated::MainPage::Create(*this, *this->bindings);
        xaml::layoutInViewport(*this->root, availableSize);
    }

    void MainPageViewModel::Update() {

    }

    xaml::Element& MainPageViewModel::Root() {
        return *this->root;
    }
#if defined(ANDROID_APP_PREVIEWER)
    xaml::runtime::RuntimeBindingContext MainPageViewModel::preview_RuntimeContext() {
        xaml::runtime::RuntimeBindingContext result;
        result.xamlNamespace = "urn:{{application}}:xaml";
        result.owner = PageName;
        result.bindings = std::make_shared<xaml::runtime::RuntimeBindingRegistry>();
        result.bindings->AddCommand("NavigateToSettingsCommand", this->NavigateToSettingsCommand());
        return result;
    }

    void MainPageViewModel::preview_ReplaceRuntimeTree(xaml::runtime::RuntimeBuildResult result) {
        this->bindings = std::move(result.bindings);
        this->root = std::move(result.root);
    }
#endif
    //
    // API
    //
    xaml::Element::Command MainPageViewModel::NavigateToSettingsCommand() {
        return [this] {
            auto state = std::make_unique<core::GreetingNavigationState>();
            state->Message = this->context.repository.Greeting();
            this->context.navigator.Trigger(core::NavigationTrigger::navigateToSettings, std::move(state));
        };
    }
}