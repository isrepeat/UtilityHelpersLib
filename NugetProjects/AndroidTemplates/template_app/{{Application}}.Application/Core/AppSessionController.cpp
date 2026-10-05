#include "AppSessionController.h"

#include <utility>

namespace {{application}}::application::core {
    //
    // IHostCommandDispatcher
    //
    void AppSessionController::Dispatch(HostCommand hostCommand, const HostCommandData& hostCommandData) {
        if (this->hostEventHandler) {
            this->hostEventHandler(hostCommand, hostCommandData);
        } else {
            this->SetStatus("Application updates are available in the Android host.");
        }
    }

    //
    // API
    //
    void AppSessionController::SetHostEventHandler(HostEventHandler hostEventHandler) {
        this->hostEventHandler = std::move(hostEventHandler);
    }

    const std::string& AppSessionController::Status() const {
        return this->status;
    }

    void AppSessionController::SetStatus(std::string value) {
        this->status = std::move(value);
    }
}