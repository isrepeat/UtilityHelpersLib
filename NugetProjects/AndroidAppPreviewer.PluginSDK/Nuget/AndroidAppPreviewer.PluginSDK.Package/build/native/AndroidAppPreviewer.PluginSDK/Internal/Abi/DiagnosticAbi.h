#pragma once
#include "../../AndroidAppPreviewerPlugin.h"

namespace preview_sdk::abi {
    using namespace AndroidAppPreviewerPluginSDK;

    class DiagnosticAbi final {
    public:
        static void xp_configure_logging(const char* filePath);
        static void xp_log_info(const char* message);
    };
}