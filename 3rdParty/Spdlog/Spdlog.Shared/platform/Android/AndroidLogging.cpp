#include "AndroidLogging.h"

#include <spdlog/sinks/android_sink.h>

#include <memory>
#include <mutex>
#include <string>

namespace utility_helpers::android {
namespace {

std::once_flag initializationFlag;
std::shared_ptr<spdlog::logger> logger;

} // namespace

void initializeLogging(std::string_view tag) {
    std::call_once(initializationFlag, [tag] {
        auto sink = std::make_shared<spdlog::sinks::android_sink_mt>(std::string(tag));
        logger = std::make_shared<spdlog::logger>("utility_helpers_android", std::move(sink));
        logger->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
        logger->set_level(spdlog::level::trace);
    });
}

spdlog::logger& log() {
    initializeLogging();
    return *logger;
}

} // namespace utility_helpers::android
