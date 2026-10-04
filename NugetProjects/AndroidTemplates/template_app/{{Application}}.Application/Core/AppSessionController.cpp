#include "AppSessionController.h"

#include <utility>

namespace {{application}}::application::core {
    //
    // IHostCommandDispatcher
    //
    void AppSessionController::Dispatch(HostCommand command, const HostCommandData& data) {
        if (this->hostEventHandler) {
            this->hostEventHandler(command, data);
        } else {
            this->SetStatus("Application updates are available in the Android host.");
        }
    }

    //
    // API
    //
    void AppSessionController::SetHostEventHandler(HostEventHandler handler) {
        this->hostEventHandler = std::move(handler);
    }

    const std::string& AppSessionController::Status() const {
        return this->status;
    }

    void AppSessionController::SetStatus(std::string value) {
        this->status = std::move(value);
    }
}