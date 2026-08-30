#if defined(__ANDROID__)

#include "Logging.h"

#include <spdlog/sinks/android_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace utility_helpers::android {
    namespace {
        std::once_flag initializationFlag;
        std::shared_ptr<spdlog::logger> logger;
        std::string logFilePath;
        LoggingOptions options;
        std::mutex configurationMutex;

        constexpr std::string_view kPattern = "[%L] [%t] %d.%m.%Y %H:%M:%S:%e {%s:%# %!} %v";
        constexpr std::string_view kSessionSeparator = "==========================================================================================================";

        spdlog::level::level_enum levelForMode(LoggingMode mode) {
            return mode == LoggingMode::Verbose ? spdlog::level::trace : spdlog::level::info;
        }
    } // namespace

    void configureLogging(LoggingOptions newOptions) {
        std::lock_guard lock(configurationMutex);
        if (!logger) options = newOptions;
    }

    void configureLogFile(std::string_view path) {
        std::lock_guard lock(configurationMutex);
        if (logger) return;
        logFilePath = path;
    }

    void initializeLogging(std::string_view tag) {
        std::call_once(initializationFlag, [tag] {
            LoggingOptions initializationOptions;
            std::string initializationLogFilePath;
            {
                std::lock_guard lock(configurationMutex);
                initializationOptions = options;
                initializationLogFilePath = logFilePath;
            }

            std::vector<spdlog::sink_ptr> sinks;
            sinks.emplace_back(std::make_shared<spdlog::sinks::android_sink_mt>(std::string(tag)));
            if (!initializationLogFilePath.empty()) {
                sinks.emplace_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    initializationLogFilePath,
                    initializationOptions.maxFileSize,
                    initializationOptions.maxFiles));
            }
            logger = std::make_shared<spdlog::logger>(
                "utility_helpers_android", sinks.begin(), sinks.end());
            logger->set_pattern(std::string(kPattern));
            logger->set_level(levelForMode(initializationOptions.mode));
            logger->flush_on(spdlog::level::trace);

            if (initializationOptions.appendNewSession) {
                logger->debug("");
                logger->debug(kSessionSeparator);
                logger->debug("                       New session started");
                logger->debug(kSessionSeparator);
            }
            });
    }

    spdlog::logger& log() {
        initializeLogging();
        return *logger;
    }

    LoggingMode loggingMode() {
        initializeLogging();
        return logger->level() == spdlog::level::trace ? LoggingMode::Verbose : LoggingMode::Normal;
    }

    void setLoggingMode(LoggingMode mode) {
        initializeLogging();
        logger->set_level(levelForMode(mode));
    }

    void flushLogging() {
        if (logger) logger->flush();
    }
} // namespace utility_helpers::android

#endif // defined(__ANDROID__)