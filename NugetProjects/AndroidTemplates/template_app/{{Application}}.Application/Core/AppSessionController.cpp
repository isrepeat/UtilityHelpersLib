#include "AppSessionController.h"

#include <utility>

namespace {{application}}::application::core {
    AppSessionController::AppSessionController(ApplicationSession& session)
        : session(session) {
    }

    //
    // API
    //
    void AppSessionController::Dispatch(AppSessionSignal signal, AppSessionSignalData data) {
        if (this->hostEventHandler) {
            this->hostEventHandler(signal, data);
        }
    }

    void AppSessionController::SetHostEventHandler(HostEventHandler handler) {
        this->hostEventHandler = std::move(handler);
    }

    ApplicationSession& AppSessionController::Session() {
        return this->session;
    }

    const ApplicationSession& AppSessionController::Session() const {
        return this->session;
    }
}