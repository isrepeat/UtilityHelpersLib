#include "SettingsPageViewModel.h"

#if defined(ANDROID_APP_PREVIEWER)
#include <XamlRuntime/RuntimeMarkup/RuntimeBindingRegistry.h>
#endif

#include "../../Core/NavigationStates.h"
#include "../../../!Generated/{{Application}}.Application/Xaml/Page/SettingsPage.xaml.h"

#include <utility>

namespace {{application}}::application::ui::page {
    SettingsPageViewModel::SettingsPageViewModel(core::PageContext& context)
        : context(context) {
    }

    //
    // INavigationPage
    //
    std::unique_ptr<base::NavigationStateBase> SettingsPageViewModel::OnNavigatingFrom(const core::NavigationRequest&) {
        return {};
    }

    bool SettingsPageViewModel::OnNavigatingTo(const core::NavigationRequest&, std::unique_ptr<base::NavigationStateBase> state) {
        const auto* greeting = dynamic_cast<const core::GreetingNavigationState*>(state.get());
        if (greeting == nullptr) {
            return false;
        }
        this->message = greeting->Message;
        for (const auto& [id, handler] : this->handlers) {
            handler(Property::message);
        }
        return true;
    }

    //
    // IPage
    //
    std::string_view SettingsPageViewModel::Name() const {
        return PageName;
    }

    std::string_view SettingsPageViewModel::Title() const {
        return "Settings";
    }

    void SettingsPageViewModel::Initialize(xaml::Size availableSize) {
        this->bindings = std::make_unique<xaml::BindingScope>();
        this->root = xaml::generated::SettingsPage::Create(*this, *this->bindings);
        xaml::layoutInViewport(*this->root, availableSize);
    }

    void SettingsPageViewModel::Update() {
    }

    xaml::Element& SettingsPageViewModel::Root() {
        return *this->root;
    }
#if defined(ANDROID_APP_PREVIEWER)
    xaml::runtime::RuntimeBindingContext SettingsPageViewModel::preview_RuntimeContext() {
        xaml::runtime::RuntimeBindingContext result;
        result.xamlNamespace = "urn:{{application}}:xaml";
        result.owner = PageName;
        result.bindings = std::make_shared<xaml::runtime::RuntimeBindingRegistry>();
        result.bindings->AddCommand("NavigateToMainCommand", this->NavigateToMainCommand());
        result.bindings->AddText("Message", [this] { return this->message; }, [this](std::function<void()> handler) {
            return this->Subscribe([handler](Property) { handler(); });
        });
        return result;
    }

    void SettingsPageViewModel::preview_ReplaceRuntimeTree(xaml::runtime::RuntimeBuildResult result) {
        this->bindings = std::move(result.bindings);
        this->root = std::move(result.root);
    }
#endif
    //
    // API
    //
    xaml::Element::Command SettingsPageViewModel::NavigateToMainCommand() {
        return [this] {
            this->context.navigator.NavigateBack();
        };
    }

    std::function<void()> SettingsPageViewModel::Subscribe(PropertyChangedHandler handler) {
        const size_t id = ++this->nextSubscription;
        this->handlers.emplace(id, std::move(handler));
        return [this, id] { this->handlers.erase(id); };
    }

    const std::string& SettingsPageViewModel::Message() const {
        return this->message;
    }
}