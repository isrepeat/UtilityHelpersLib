#pragma once

#if defined(__ANDROID__)

#include <spdlog/logger.h>

#include <cstddef>
#include <string>
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

    // RAII-логгер области: пишет вход при создании и выход при разрушении.
    class FunctionScope final {
    public:
        FunctionScope(spdlog::source_loc sourceLocation, std::string functionName);
        ~FunctionScope();

        FunctionScope(const FunctionScope&) = delete;
        FunctionScope& operator=(const FunctionScope&) = delete;

    private:
        spdlog::source_loc sourceLocation_;
        std::string functionName_;
    };
} // namespace utility_helpers::android

// Передаём координаты вызова в spdlog, чтобы Logcat и файл содержали место
// возникновения сообщения, а не строку внутри реализации логгера.
#define LOG(logLevel, ...) \
    ::utility_helpers::android::log().log( \
        spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, \
        spdlog::level::level_enum::logLevel, \
        __VA_ARGS__)

#define LOG_TRACE(...) LOG(trace, __VA_ARGS__)
#define LOG_DEBUG(...) LOG(debug, __VA_ARGS__)
#define LOG_INFO(...) LOG(info, __VA_ARGS__)
#define LOG_WARNING(...) LOG(warn, __VA_ARGS__)
#define LOG_ERROR(...) LOG(err, __VA_ARGS__)

#define UTILITY_HELPERS_ANDROID_LOG_CONCAT_INNER(left, right) left##right
#define UTILITY_HELPERS_ANDROID_LOG_CONCAT(left, right) \
    UTILITY_HELPERS_ANDROID_LOG_CONCAT_INNER(left, right)

// Аналог LOG_FUNCTION_SCOPE из Windows LogHelpers: сообщение формируется один
// раз, а деструктор FunctionScope гарантированно логирует выход при return.
#define LOG_FUNCTION_SCOPE(...) \
    ::utility_helpers::android::FunctionScope \
        UTILITY_HELPERS_ANDROID_LOG_CONCAT(functionScopeLog_, __COUNTER__)( \
            spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, \
            fmt::format(__VA_ARGS__))

#endif // defined(__ANDROID__)