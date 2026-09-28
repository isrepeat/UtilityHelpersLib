#include "PreviewNavigationController.h"

namespace {{application}}::preview::session {
    //
    // API
    //
    std::string PreviewNavigationController::BuildGraphJson() const {
        return R"({"currentPageId":"MainPage","layoutRootPageId":"MainPage","pages":[{"id":"MainPage","title":"MainPage"}],"transitions":[]})";
    }

    bool PreviewNavigationController::Navigate(std::span<const std::string_view> transitionIds, std::string& error) const {
        if (!transitionIds.empty()) {
            error = "The template does not define navigation transitions";
            return false;
        }
        error.clear();
        return true;
    }
}