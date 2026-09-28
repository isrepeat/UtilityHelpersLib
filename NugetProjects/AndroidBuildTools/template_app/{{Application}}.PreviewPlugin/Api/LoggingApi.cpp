#include "LoggingApi.h"

#include <Helpers.Logging/Logging.h>

#include <filesystem>

namespace {{application}}::preview::api {
    using namespace AndroidAppPreviewerPluginSDK;
    //
    // API
    //
    void LoggingApi::xp_configure_logging(const char* filePath) {
        utility_helpers::logging::Configure({
            filePath == nullptr ? std::filesystem::path{} : std::filesystem::path(filePath),
        });
        utility_helpers::logging::Initialize("AndroidAppPreviewer");
        LOG_INFO("{{Application}}.PreviewPlugin", "Logging initialized");
    }

    void LoggingApi::xp_log_info(const char* message) {
        if (message != nullptr) {
            LOG_INFO("AndroidAppPreviewer.Interaction", "{}", message);
        }
    }
} // namespace {{application}}::preview::api