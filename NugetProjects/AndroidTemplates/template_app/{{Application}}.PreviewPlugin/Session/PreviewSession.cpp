#include "PreviewSession.h"

#include <XamlRuntime/RuntimeMarkup/RuntimeBindingRegistry.h>
#include <XamlRuntime/RuntimeMarkup/RuntimeTreeBuilder.h>
#include <XamlRuntime/RuntimeMarkup/XamlParser.h>

#include "../../!Generated/{{Application}}.Application/Xaml/Page/SettingsPage.xaml.h"
#include "../../!Generated/{{Application}}.Application/Xaml/Page/MainPage.xaml.h"

#include <stdexcept>
#include <utility>
#include <cctype>
#include <vector>

namespace {{application}}::preview::session {
    PreviewSession::PreviewSession(int width, int height)
        : navigation(*this) {
        if (width <= 0 || height <= 0) {
            throw std::invalid_argument("Preview session dimensions must be positive");
        }
        this->viewport = {static_cast<float>(width), static_cast<float>(height)};
        this->viewModel.session = this;
        this->LoadPage("MainPage");
    }

    //
    // API
    //
    xaml::Element& PreviewSession::Root() {
        return *this->root;
    }

    PreviewNavigationController& PreviewSession::Navigation() {
        return this->navigation;
    }

    std::string_view PreviewSession::CurrentPage() const {
        return this->currentPage;
    }

    std::string_view PreviewSession::PageTitle(std::string_view page) const {
        if (page == "MainPage") {
            return "Главная";
        }
        if (page == "SettingsPage") {
            return "Настройки";
        }
        return {};
    }

    bool PreviewSession::LoadPage(std::string_view page) {
        if (!this->IsKnownPage(page)) {
            return false;
        }
        auto bindings = std::make_unique<xaml::BindingScope>();
        std::unique_ptr<xaml::Element> root;
        if (page == "MainPage") {
            root = xaml::generated::MainPage::Create(this->viewModel, *bindings);
        } else {
            root = xaml::generated::SettingsPage::Create(this->viewModel, *bindings);
        }
        xaml::AnimationController animations;
        animations.Attach(*root, xaml::AnimationRegistry{}, false);
        this->interaction.Cancel();
        this->animations = std::move(animations);
        this->bindings = std::move(bindings);
        this->root = std::move(root);
        this->currentPage = page;
        xaml::layoutInViewport(*this->root, this->viewport);
        return true;
    }

    void PreviewSession::QueuePage(std::string_view page) {
        if (this->IsKnownPage(page)) {
            this->pendingPage = page;
        }
    }

    void PreviewSession::Resize(int width, int height) {
        if (width <= 0 || height <= 0) {
            throw std::invalid_argument("Preview session dimensions must be positive");
        }
        this->viewport = {static_cast<float>(width), static_cast<float>(height)};
        xaml::layoutInViewport(*this->root, this->viewport);
    }

    bool PreviewSession::ReloadMarkup(std::string_view page, std::string_view markup, std::string_view sourcePath, std::string& error) {
        if (!this->IsKnownPage(page)) {
            error = "Unknown template page";
            return false;
        }
        xaml::runtime::RuntimeBindingContext context;
        context.xamlNamespace = "urn:{{application}}:xaml";
        context.owner = page;
        context.bindings = std::make_shared<xaml::runtime::RuntimeBindingRegistry>();
        context.bindings->AddCommand("NavigateToMainCommand", [this] { this->QueuePage("MainPage"); });
        context.bindings->AddCommand("NavigateToSettingsCommand", [this] { this->QueuePage("SettingsPage"); });
        context.bindings->AddCommand("UpdateApplicationCommand", [] {});
        auto tree = xaml::runtime::RuntimeTreeBuilder{}.BuildPage(
            xaml::runtime::XamlParser{}.Parse(markup, sourcePath), context, this->viewport);
        xaml::AnimationController animations;
        animations.Attach(*tree.root, xaml::AnimationRegistry{}, false);
        this->interaction.Cancel();
        this->animations = std::move(animations);
        this->bindings = std::move(tree.bindings);
        this->root = std::move(tree.root);
        this->currentPage = page;
        error.clear();
        return true;
    }

    void PreviewSession::SetAnimationPlaybackRate(float value) {
        this->animations.SetPlaybackRate(value);
    }

    void PreviewSession::PointerDown(float x, float y) {
        this->interaction.PointerDown(*this->root, this->animations, x, y);
    }

    void PreviewSession::PointerMove(float x, float y) {
        this->interaction.PointerMove(x, y);
    }

    void PreviewSession::PointerUp(float x, float y) {
        this->interaction.PointerUp(*this->root, this->animations, x, y);
    }

    void PreviewSession::CancelPointer() {
        this->interaction.Cancel();
    }

    bool PreviewSession::Update() {
        const bool interacting = this->interaction.Update();
        const bool animating = this->animations.IsAnimating();
        this->animations.Update();
        if (!this->pendingPage.empty()) {
            const std::string page = std::move(this->pendingPage);
            this->pendingPage.clear();
            this->LoadPage(page);
            return true;
        }
        xaml::layoutInViewport(*this->root, this->viewport);
        return interacting || animating || this->animations.IsAnimating();
    }

    bool PreviewSession::IsKnownPage(std::string_view page) const {
        return !this->PageTitle(page).empty();
    }

    xaml::Element::Command PreviewSession::ViewModel::NavigateToMainCommand() {
        return [this] { this->session->QueuePage("MainPage"); };
    }

    xaml::Element::Command PreviewSession::ViewModel::NavigateToSettingsCommand() {
        return [this] { this->session->QueuePage("SettingsPage"); };
    }

    xaml::Element::Command PreviewSession::ViewModel::UpdateApplicationCommand() {
        return [] {};
    }

    std::vector<std::string> PreviewSession::ParseNavigationTransitionIds(std::string_view json) {
        const size_t property = json.find("\"transitionIds\"");
        if (property == std::string_view::npos) {
            throw std::invalid_argument("Navigation request does not contain transitionIds");
        }
        const size_t arrayStart = json.find('[', property);
        const size_t arrayEnd = arrayStart == std::string_view::npos ? std::string_view::npos : json.find(']', arrayStart);
        if (arrayStart == std::string_view::npos || arrayEnd == std::string_view::npos) {
            throw std::invalid_argument("Navigation request contains an invalid transitionIds array");
        }
        std::vector<std::string> result;
        size_t position = arrayStart + 1;
        while (position < arrayEnd) {
            while (position < arrayEnd && std::isspace(static_cast<unsigned char>(json[position]))) {
                ++position;
            }
            if (position == arrayEnd) {
                break;
            }
            if (json[position] != '\"') {
                throw std::invalid_argument("Navigation transition ID must be a JSON string");
            }
            const size_t valueStart = ++position;
            const size_t valueEnd = json.find('\"', valueStart);
            if (valueEnd == std::string_view::npos || valueEnd > arrayEnd) {
                throw std::invalid_argument("Navigation transition ID is not terminated");
            }
            if (json.substr(valueStart, valueEnd - valueStart).find('\\') != std::string_view::npos) {
                throw std::invalid_argument("Navigation transition ID must not contain JSON escapes");
            }
            result.emplace_back(json.substr(valueStart, valueEnd - valueStart));
            position = valueEnd + 1;
            while (position < arrayEnd && std::isspace(static_cast<unsigned char>(json[position]))) {
                ++position;
            }
            if (position < arrayEnd) {
                if (json[position] != ',') {
                    throw std::invalid_argument("Navigation transition IDs must be comma-separated");
                }
                ++position;
            }
        }
        if (result.empty()) {
            throw std::invalid_argument("Navigation request does not contain transition IDs");
        }
        return result;
    }
}