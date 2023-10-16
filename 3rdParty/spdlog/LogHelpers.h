#pragma once
// define these macros before first include spdlog headers
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT 
#define SPDLOG_WCHAR_FILENAMES
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include "spdlog/sinks/msvc_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#ifdef INSIDE_HELPERS_PROJECT
#include "Scope.h"
#include "Macros.h"
#include "String.hpp"
#include "TypeTraits.hpp"
#else
#include <Helpers/Scope.h>
#include <Helpers/Macros.h>
#include <Helpers/String.hpp>
#include <Helpers/TypeTraits.hpp>
#endif
#include <unordered_map>
#include <string>
#include <memory>


namespace lg {
    // define a "__classFullnameLogging" "member checker" class
    define_has_member(__ClassFullnameLogging);

    // TODO: rewrite singleton as shared_ptr to destroy it after all others singletons
    class DefaultLoggers {
    private:
        DefaultLoggers();
        static DefaultLoggers& GetInstance();
    public:
        ~DefaultLoggers() = default;
        struct UnscopedData;

        static void Init(const std::wstring& logFilePath, bool truncate = false, bool appendNewSessionMsg = true);
        static std::shared_ptr<spdlog::logger> Logger();
        static std::shared_ptr<spdlog::logger> RawLogger();
        static std::shared_ptr<spdlog::logger> TimeLogger();
        static std::shared_ptr<spdlog::logger> FuncLogger();
        static std::shared_ptr<spdlog::logger> DebugLogger();

        // Use _impl to deduct T in std::basic_string<T>
        template<typename TClass, typename T, typename... Args>
        static void Log_impl(
            TClass* classPtr,
            std::shared_ptr<spdlog::logger> logger,
            spdlog::source_loc location, spdlog::level::level_enum level, std::basic_string<T> format, Args&&... args)
        {
            if constexpr (has_member(std::remove_reference_t<decltype(std::declval<TClass>())>, __ClassFullnameLogging)) {
                if constexpr (std::is_same_v<T, char>) {
                    (logger)->log(location, level, "[{}] " + format, classPtr->GetFullClassNameA(), std::forward<Args&&>(args)...);
                }
                else {
                    (logger)->log(location, level, L"[{}] " + format, classPtr->GetFullClassNameW(), std::forward<Args&&>(args)...);
                }
                return;
            }
            (logger)->log(location, level, format, std::forward<Args&&>(args)...);
        }

        template<typename TClass, typename TFormat, typename... Args>
        static void Log(
            TClass* classPtr,
            std::shared_ptr<spdlog::logger> logger,
            spdlog::source_loc location, spdlog::level::level_enum level, TFormat format, Args&&... args)
        {
            using T = typename decltype(H::StringDeductor(format))::type;
            Log_impl<TClass, T, Args...>(classPtr, logger, location, level, format, std::forward<Args&&>(args)...);
        }

    private:
        std::shared_ptr<spdlog::logger> logger;
        std::shared_ptr<spdlog::logger> rawLogger;
        std::shared_ptr<spdlog::logger> timeLogger;
        std::shared_ptr<spdlog::logger> funcLogger;

        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSink;
        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSinkRaw;
        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSinkTime;
        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSinkFunc;

#ifdef _DEBUG
        std::shared_ptr<spdlog::logger> debugLogger;
        const std::shared_ptr<spdlog::sinks::msvc_sink_mt> debugSink;
#endif
        std::shared_ptr<int> token = std::make_shared<int>();
    };

    constexpr H::nothing* nullctx = nullptr; // explicilty pass null ctx for logger
}

// TODO: find better solution to detect class context
H::nothing* __LgCtx(); // may be overwritten as class method that returned "this" (throught class fullname logging macro)


#if defined(LOG_RAW)
#error LOG_... macros already defined
#endif
#if defined(LOG_INFO) || defined(LOG_DEBUG) || defined(LOG_ERROR) || defined(LOG_WARNING)
#error LOG_... macros already defined
#endif
#if defined(LOG_INFO_D) || defined(LOG_DEBUG_D) || defined(LOG_ERROR_D) || defined(LOG_WARNING_D)
#error LOG_... macros already defined
#endif


#if !defined(DISABLE_COMMON_LOGGING)
#define LOG_CTX spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}

#define LOG_RAW(...) lg::DefaultLoggers::Log(__LgCtx(), lg::DefaultLoggers::RawLogger(), LOG_CTX, spdlog::level::trace, __VA_ARGS__)
#define LOG_TIME(...) lg::DefaultLoggers::Log(__LgCtx(), lg::DefaultLoggers::TimeLogger(), LOG_CTX, spdlog::level::trace, __VA_ARGS__)

#define LOG_INFO(...) lg::DefaultLoggers::Log(__LgCtx(), lg::DefaultLoggers::Logger(), LOG_CTX, spdlog::level::info, __VA_ARGS__)
#define LOG_DEBUG(...) lg::DefaultLoggers::Log(__LgCtx(), lg::DefaultLoggers::Logger(), LOG_CTX, spdlog::level::debug, __VA_ARGS__)
#define LOG_ERROR(...) lg::DefaultLoggers::Log(__LgCtx(), lg::DefaultLoggers::Logger(), LOG_CTX, spdlog::level::err, __VA_ARGS__)
#define LOG_WARNING(...) lg::DefaultLoggers::Log(__LgCtx(), lg::DefaultLoggers::Logger(), LOG_CTX, spdlog::level::warn, __VA_ARGS__)

#define LOG_INFO_D(...) lg::DefaultLoggers::Log(__LgCtx(), lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::info, __VA_ARGS__)
#define LOG_DEBUG_D(...) lg::DefaultLoggers::Log(__LgCtx(), lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::debug, __VA_ARGS__)
#define LOG_ERROR_D(...) lg::DefaultLoggers::Log(__LgCtx(), lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::err, __VA_ARGS__)
#define LOG_WARNING_D(...) lg::DefaultLoggers::Log(__LgCtx(), lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::warn, __VA_ARGS__)

// Use it inside static functions or with custom context:
#define LOG_INFO_S(_This, ...) lg::DefaultLoggers::Log(_This, lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::info, __VA_ARGS__)
#define LOG_DEBUG_S(_This, ...) lg::DefaultLoggers::Log(_This, lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::debug, __VA_ARGS__)
#define LOG_ERROR_S(_This, ...) lg::DefaultLoggers::Log(_This, lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::err, __VA_ARGS__)
#define LOG_WARNING_S(_This, ...) lg::DefaultLoggers::Log(_This, lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::warn, __VA_ARGS__)


#define LOG_FUNCTION_ENTER(...) lg::DefaultLoggers::Log(lg::nullctx, lg::DefaultLoggers::FuncLogger(), LOG_CTX, spdlog::level::debug, __VA_ARGS__)

#define LOG_FUNCTION_SCOPE(format, ...)                                                                                      \
	LOG_FUNCTION_ENTER(EXPAND_1_VA_ARGS_(format, __VA_ARGS__));                                                              \
                                                                                                                             \
	auto functionFinishLogScoped = H::MakeScope([&] {                                                                        \
		LOG_FUNCTION_ENTER(std::string("~") + format); /* TODO: add suppoort for wstr */                                     \
		});

#else
#define LOG_CTX
#define LOG_RAW(...)
#define LOG_TIME(...)
#define LOG_INFO(...)
#define LOG_DEBUG(...)
#define LOG_ERROR(...)
#define LOG_WARNING(...)
#define LOG_INFO_D(...)
#define LOG_DEBUG_D(...)
#define LOG_ERROR_D(...)
#define LOG_WARNING_D(...)
#define LOG_FUNCTION_ENTER(...)
#define LOG_FUNCTION_SCOPE(format, ...)
#endif


#if !defined(DISABLE_CLASS_FULLNAME_LOGGING)
#define CLASS_FULLNAME_LOGGING_INLINE_IMPLEMENTATION(className)                                                               \
	private:                                                                                                                  \
		std::string className##_fullClassNameA = ""#className;                                                                \
		std::wstring className##_fullClassNameW = L""#className;                                                              \
                                                                                                                              \
	public:                                                                                                                   \
	    void __ClassFullnameLogging() {}                                                                                      \
	                                                                                                                          \
	    className* __LgCtx() {                                                                                                \
            return this;                                                                                                      \
        }                                                                                                                     \
	                                                                                                                          \
        void SetFullClassName(std::wstring name) {                                                                            \
            LOG_DEBUG_D(L"Full class name = {}", name);                                                                       \
            this->className##_fullClassNameA = H::WStrToStr(name);                                                            \
            this->className##_fullClassNameW = name;                                                                          \
        }                                                                                                                     \
                                                                                                                              \
		const std::string& GetFullClassNameA() {                                                                              \
			return this->className##_fullClassNameA;                                                                          \
		}                                                                                                                     \
                                                                                                                              \
		const std::wstring& GetFullClassNameW() {                                                                             \
			return this->className##_fullClassNameW;                                                                          \
		}

#else
#define CLASS_FULLNAME_LOGGING_INLINE_IMPLEMENTATION(className)
#endif