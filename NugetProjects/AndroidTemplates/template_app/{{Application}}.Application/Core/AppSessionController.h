#pragma once
#include "./ApplicationSession.h"

#include <functional>

namespace {{application}}::application::core {
    enum class AppSessionSignal { closeApplication };
    struct AppSessionSignalData final {
        std::string value;
    };

    class AppSessionController final {
    public:
        using HostEventHandler = std::function<void(AppSessionSignal, const AppSessionSignalData&)>;
        explicit AppSessionController(ApplicationSession& session);
        void Dispatch(AppSessionSignal signal, AppSessionSignalData data);
        void SetHostEventHandler(HostEventHandler handler);
        ApplicationSession& Session();
        const ApplicationSession& Session() const;

    private:
        ApplicationSession& session;
        HostEventHandler hostEventHandler;
    };
}