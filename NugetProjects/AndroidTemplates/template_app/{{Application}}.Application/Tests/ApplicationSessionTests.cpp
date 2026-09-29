#include "../Core/ApplicationSession.h"
#include "../Core/NavigationStates.h"

#include <stdexcept>
#include <iostream>
#include <array>

namespace _details {
    void Require(bool condition, const char* message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    xaml::Element& Find(xaml::Element& root, std::string_view id) {
        if (root.Id() == id) {
            return root;
        }
        for (const auto& child : root.Children()) {
            try {
                return Find(*child, id);
            } catch (const std::out_of_range&) {
            }
        }
        throw std::out_of_range("Element not found");
    }

    class WrongState final : public {{application}}::application::base::NavigationStateBase {
    public:
        std::string_view TypeId() const override;
    };

    std::string_view WrongState::TypeId() const {
        return "WrongState";
    }
}

int main() {
    using namespace {{application}}::application;
    try {
        core::ApplicationSession session;
        session.Initialize({480, 800});
        auto& pages = session.Pages();
        auto* mainRoot = &session.Root();
        _details::Require(!pages.NavigateBack(), "Root cannot go back");
        _details::Require(!pages.Trigger(core::NavigationTrigger::navigateToSettings), "Required payload accepted when missing");
        _details::Require(!pages.Trigger(core::NavigationTrigger::navigateToSettings, std::make_unique<_details::WrongState>()), "Wrong TypeId accepted");
        _details::Require(pages.CurrentPageName() == "MainPage", "Rejected transition mutated current page");
        session.Repository().SetGreeting("Payload from Main");
        const auto bounds = _details::Find(session.Root(), "openSettingsButton").Bounds();
        session.PointerDown(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);
        session.PointerUp(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);
        _details::Require(pages.CurrentPageName() == "SettingsPage", "Pointer command did not navigate");
        _details::Require(_details::Find(session.Root(), "greetingText").Text() == "Payload from Main", "Typed payload did not reach XAML binding");
        auto* settingsRoot = &session.Root();
        _details::Find(session.Root(), "backButton").ExecuteCommand();
        _details::Require(pages.CurrentPageName() == "MainPage" && &session.Root() == mainRoot, "Back did not retain registered page");
        std::string error;
        const std::array forward{std::string_view{"main-to-settings"}};
        const std::array back{std::string_view{"settings-to-main"}};
        _details::Require(pages.preview_NavigateTransitions(forward, error), error.c_str());
        _details::Require(&session.Root() == settingsRoot, "Settings was recreated");
        _details::Require(_details::Find(session.Root(), "greetingText").Text() == "Hello from preview!", "Native preview default missing");
        _details::Require(pages.preview_NavigateTransitions(back, error), error.c_str());
        const std::array invalid{std::string_view{"main-to-settings"}, std::string_view{"unknown"}};
        _details::Require(!pages.preview_NavigateTransitions(invalid, error) && pages.CurrentPageName() == "MainPage", "Invalid path partially changed history");
        const std::array roundTrip{std::string_view{"main-to-settings"}, std::string_view{"settings-to-main"}, std::string_view{"main-to-settings"}};
        _details::Require(pages.preview_NavigateTransitions(roundTrip, error), error.c_str());
        const auto markup = R"(<Page xmlns="urn:{{application}}:xaml"><StackPanel><TextBlock id="greetingText" text="{Binding Message}"/><Button id="backButton" command="{Binding NavigateToMainCommand}" text="Back"/></StackPanel></Page>)";
        _details::Require(pages.preview_ReloadMarkup("SettingsPage", markup, "SettingsPage.xaml", error), error.c_str());
        _details::Require(_details::Find(session.Root(), "greetingText").Text() == "Hello from preview!", "Reload lost binding state");
        _details::Find(session.Root(), "backButton").ExecuteCommand();
        _details::Require(pages.CurrentPageName() == "MainPage", "Reload discarded history or command");
        session.Repository().SetGreeting("After reload");
        _details::Find(session.Root(), "openSettingsButton").ExecuteCommand();
        _details::Require(_details::Find(session.Root(), "greetingText").Text() == "After reload", "Runtime binding did not receive new payload");
        _details::Require(pages.NavigateBack() && !pages.NavigateBack(), "History contains duplicate roots");
        const auto routes = pages.preview_Routes();
        _details::Require(routes.size() == 2 && routes[0].dataType == core::GreetingNavigationState::DataTypeId
            && routes[1].targetKind == core::NavigationTargetKind::previousPage, "Graph lost typed or backward route");
        core::ApplicationSession rejectedSave({}, [](const model::ApplicationStateDocument&) { return false; });
        _details::Require(!rejectedSave.Repository().SetGreeting("Unsaved") && rejectedSave.Repository().Greeting() == "Hello from Main!", "Failed save changed repository");
        std::cout << "PASS: pointer navigation, required payload, TypeId, lifecycle, history, native defaults, hot reload, persistence" << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}