#include "PreviewNavigationController.h"

#include "./PreviewSession.h"

#include <set>

namespace {{application}}::preview::session {
    namespace _details {
        std::string JsonString(std::string_view value) {
            std::string result = "\"";
            constexpr char digits[] = "0123456789abcdef";
            for (const unsigned char c : value) {
                if (c == '"' || c == '\\') {
                    result += '\\';
                    result += c;
                } else if (c < 32) {
                    result += "\\u00";
                    result += digits[c >> 4];
                    result += digits[c & 15];
                } else {
                    result += c;
                }
            }
            return result + '"';
        }
    }

    PreviewNavigationController::PreviewNavigationController(PreviewSession& session)
        : session(session) {
    }

    //
    // API
    //
    std::string PreviewNavigationController::BuildGraphJson() const {
        const auto routes = this->session.Pages().preview_Routes();
        std::set<std::string_view> pages;
        pages.insert(this->session.CurrentPage());
        for (const auto& route : routes) {
            pages.insert(route.source);
            pages.insert(route.target);
        }
        std::string result = "{\"currentPageId\":" + _details::JsonString(this->session.CurrentPage())
            + ",\"layoutRootPageId\":\"MainPage\",\"pages\":[";
        bool first = true;
        for (const auto page : pages) {
            if (!first) {
                result += ',';
            }
            first = false;
            result += "{\"id\":" + _details::JsonString(page) + ",\"title\":" + _details::JsonString(this->session.PageTitle(page)) + '}';
        }
        result += "],\"transitions\":[";
        first = true;
        for (const auto& route : routes) {
            if (!first) {
                result += ',';
            }
            first = false;
            result += "{\"id\":" + _details::JsonString(route.id)
                + ",\"sourcePageId\":" + _details::JsonString(route.source)
                + ",\"targetPageId\":" + _details::JsonString(route.target)
                + ",\"backwardOfTransitionId\":" + _details::JsonString(route.backwardOfRouteId)
                + ",\"title\":" + _details::JsonString(route.title)
                + ",\"targetKind\":" + _details::JsonString(
                    route.targetKind == {{application}}::application::core::NavigationTargetKind::previousPage
                        ? "previousPage"
                        : "page")
                + ",\"isDefault\":" + (route.isDefault ? "true" : "false")
                + ",\"dataType\":" + _details::JsonString(route.dataType)
                + ",\"hasPreviewDefaultNavigationState\":" + (route.hasPreviewDefaultNavigationState ? "true" : "null") + '}';
        }
        return result + "]}";
    }

    bool PreviewNavigationController::Navigate(std::span<const std::string_view> transitionIds, std::string& error) {
        return this->session.Navigate(transitionIds, error);
    }
}