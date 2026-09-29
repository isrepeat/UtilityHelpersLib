#include "PreviewNavigationController.h"

#include "./PreviewSession.h"

namespace {{application}}::preview::session {
    PreviewNavigationController::PreviewNavigationController(PreviewSession& session)
        : session(session) {
    }

    //
    // API
    //
    std::string PreviewNavigationController::BuildGraphJson() const {
        return std::string{"{\"currentPageId\":\""} + std::string{this->session.CurrentPage()}
            + R"(","layoutRootPageId":"MainPage","pages":[{"id":"MainPage","title":"Главная"},{"id":"SettingsPage","title":"Настройки"}],"transitions":[{"id":"main-to-settings","sourcePageId":"MainPage","targetPageId":"SettingsPage","targetKind":"page","backwardOfTransitionId":"","title":"Открыть настройки","isDefault":true,"dataType":"","previewDefault":true},{"id":"settings-to-main","sourcePageId":"SettingsPage","targetPageId":"MainPage","targetKind":"page","backwardOfTransitionId":"main-to-settings","title":"Назад","isDefault":true,"dataType":"","previewDefault":true}]})";
    }

    bool PreviewNavigationController::Navigate(std::span<const std::string_view> transitionIds, std::string& error) {
        if (transitionIds.size() != 1) {
            error = "The template navigation requires exactly one transition";
            return false;
        }
        const std::string_view transition = transitionIds.front();
        if (transition == "main-to-settings" && this->session.CurrentPage() == "MainPage") {
            this->session.LoadPage("SettingsPage");
            error.clear();
            return true;
        }
        if (transition == "settings-to-main" && this->session.CurrentPage() == "SettingsPage") {
            this->session.LoadPage("MainPage");
            error.clear();
            return true;
        }
        error = "Transition is not available on the current page";
        return false;
    }
}