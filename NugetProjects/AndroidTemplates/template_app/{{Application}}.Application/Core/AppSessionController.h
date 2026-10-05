#pragma once
#include "../Interface/IHostCommandDispatcher.h"

#include <functional>

namespace {{application}}::application::core {
    class AppSessionController final : public interface::IHostCommandDispatcher {
    public:
        using HostEventHandler = std::function<void(HostCommand, const HostCommandData&)>;
        AppSessionController() = default;
        ~AppSessionController() = default;

        //
        // IHostCommandDispatcher
        //
        void Dispatch(HostCommand hostCommand, const HostCommandData& hostCommandData = {}) override;

        void SetHostEventHandler(HostEventHandler hostEventHandler);
        const std::string& Status() const;
        void SetStatus(std::string value);

    private:
        HostEventHandler hostEventHandler;
        std::string status;
    };
}