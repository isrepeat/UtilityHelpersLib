#pragma once

#include <spdlog/logger.h>

#include <cstddef>
#include <string_view>

namespace utility_helpers::android {
    enum class LoggingMode {
        // Записывает info, warn, error и critical.
        Normal,

        // Записывает всё, включая trace и debug.
        Verbose,
    };

    struct LoggingOptions {
        static constexpr std::size_t defaultMaxFileSize = 2 * 1024 * 1024;
        static constexpr std::size_t defaultMaxFiles = 3;

        LoggingMode mode = LoggingMode::Verbose;
        std::size_t maxFileSize = defaultMaxFileSize;
        std::size_t maxFiles = defaultMaxFiles;
        bool appendNewSession = true;
    };

    // Настраивает режим, ротацию и разделитель сессии до первого initializeLogging().
    void configureLogging(LoggingOptions options);

    // Задаёт путь к локальному rotating-файлу до первого initializeLogging().
    // Пустой путь оставляет только Android Logcat.
    void configureLogFile(std::string_view path);

    // Создаёт process-wide логгер с выводом в Android Logcat и, при настройке,
    // в локальный rotating-файл. Повторный вызов безопасен.
    void initializeLogging(std::string_view tag = "UtilityHelpers");

    // Возвращает логгер, создавая его с тегом по умолчанию при первом обращении.
    spdlog::logger& log();

    LoggingMode loggingMode();
    void setLoggingMode(LoggingMode mode);

    // Принудительно сбрасывает буферы перед экспортом файла в Kotlin.
    void flushLogging();
} // namespace utility_helpers::android