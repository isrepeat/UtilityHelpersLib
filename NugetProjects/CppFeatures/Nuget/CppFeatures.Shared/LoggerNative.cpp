#include "LoggerNative.h"
#define LOGGER_NS_ALIAS lgDesktop
#include "Helpers/Logger.h"

namespace CppFeatures {
    void Logger::Init(const std::wstring& logFilePath, InitFlags initFlags) {
        lgDesktop::DefaultLoggers::Init(logFilePath, static_cast<lgDesktop::InitFlags>(initFlags)); // lg::InitFlags is spdlog enum
    }

    void Logger::Log(Context context, Pattern pattern, Level level, const string_t& format) {
        auto srcLoc = spdlog::source_loc{ context.filename.c_str(), context.line, context.function.c_str() };
        auto lvl = static_cast<spdlog::level::level_enum>(level);

        switch (pattern) {
        case Pattern::Default:
            lgDesktop::DefaultLoggers::Logger()->log(srcLoc, lvl, format);
            break;
        case Pattern::Debug:
            lgDesktop::DefaultLoggers::DebugLogger()->log(srcLoc, lvl, format);
            break;
        case Pattern::Func:
            lgDesktop::DefaultLoggers::FuncLogger()->log(srcLoc, lvl, format);
            break;
        case Pattern::Raw:
            lgDesktop::DefaultLoggers::RawLogger()->log(srcLoc, lvl, format);
            break;
        }
    }
}