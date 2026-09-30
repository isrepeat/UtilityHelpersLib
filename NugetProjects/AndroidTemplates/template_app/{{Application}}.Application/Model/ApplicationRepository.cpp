#include "ApplicationRepository.h"

#include <utility>

namespace {{application}}::application::model {
    ApplicationRepository::ApplicationRepository(core::ApplicationStateStore& store)
        : base::AppRepositoryBase(store) {
    }
#if defined(ANDROID_APP_PREVIEWER)
    //
    // AppRepositoryBase
    //
    bool ApplicationRepository::preview_IsSessionDocumentEquivalentTo(const ApplicationStateDocument& document) const {
        return this->base::AppRepositoryBase::State() == document;
    }

    void ApplicationRepository::preview_ReloadFromStateStore() {
        // Репозиторий читает документ напрямую, отдельного кэша нет.
    }
#endif
    //
    // API
    //
    const std::string& ApplicationRepository::Greeting() const {
        return this->base::AppRepositoryBase::State().greeting;
    }

    bool ApplicationRepository::SetGreeting(std::string value) {
        auto document = this->base::AppRepositoryBase::State();
        document.greeting = std::move(value);
        return this->base::AppRepositoryBase::Commit(std::move(document));
    }
}