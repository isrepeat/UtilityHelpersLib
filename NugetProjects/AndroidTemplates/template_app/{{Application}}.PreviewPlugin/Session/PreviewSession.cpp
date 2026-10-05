#include "PreviewSession.h"

#include <stdexcept>
#include <cctype>

namespace {{application}}::preview::session {
    PreviewSession::PreviewSession(int width, int height)
        : navigation(*this) {
        if (width <= 0 || height <= 0) {
            throw std::invalid_argument("Preview session dimensions must be positive");
        }
        this->applicationSession.Initialize({static_cast<float>(width), static_cast<float>(height)});
    }

    //
    // API
    //
    xaml::Element& PreviewSession::Root() {
        return this->applicationSession.Root();
    }

    PreviewNavigationController& PreviewSession::Navigation() {
        return this->navigation;
    }

    const {{application}}::application::core::PageManager& PreviewSession::Pages() const {
        return this->applicationSession.Pages();
    }

    bool PreviewSession::Navigate(std::span<const std::string_view> ids, std::string& error) {
        return this->applicationSession.Pages().preview_NavigateTransitions(ids, error);
    }

    std::string_view PreviewSession::CurrentPage() const {
        return this->Pages().CurrentPageName();
    }

    std::string_view PreviewSession::PageTitle(std::string_view page) const {
        return this->Pages().PageTitle(page);
    }

    bool PreviewSession::LoadPage(std::string_view page) {
        return this->applicationSession.Pages().Navigate(page);
    }

    void PreviewSession::Resize(int width, int height) {
        this->applicationSession.Resize({static_cast<float>(width), static_cast<float>(height)});
    }

    bool PreviewSession::ReloadMarkup(std::string_view page, std::string_view markup, std::string_view sourcePath, std::string& error) {
        return this->applicationSession.Pages().preview_ReloadMarkup(page, markup, sourcePath, error);
    }

    void PreviewSession::SetAnimationPlaybackRate(float value) {
        this->applicationSession.Pages().SetAnimationPlaybackRate(value);
    }

    void PreviewSession::PointerDown(float x, float y) {
        this->applicationSession.PointerDown(x, y);
    }

    void PreviewSession::PointerMove(float x, float y) {
        this->applicationSession.PointerMove(x, y);
    }

    void PreviewSession::PointerUp(float x, float y) {
        this->applicationSession.PointerUp(x, y);
    }

    void PreviewSession::CancelPointer() {
        this->applicationSession.CancelPointer();
    }

    bool PreviewSession::Update() {
        return this->applicationSession.Update();
    }

    void PreviewSession::Render(xaml::IRenderBackend& renderer) {
        this->applicationSession.Render(renderer);
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