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

        template<typename T, typename TClass, typename... Args>
        static void Log(
            TClass* classPtr,
            std::shared_ptr<spdlog::logger> logger,
            spdlog::source_loc location, spdlog::level::level_enum level, fmt::basic_format_string<T, std::type_identity_t<Args>...> format, Args&&... args)
        {
            if constexpr (has_member(std::remove_reference_t<decltype(std::declval<TClass>())>, __ClassFullnameLogging)) {
                GetInstance().className = L" [" + classPtr->GetFullClassNameW() + L"]";
            }
            else {
                GetInstance().className = L"";
            }
            (logger)->log(location, level, format, std::forward<Args&&>(args)...);
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

        // Custom prefix
        std::wstring className = L"";
        std::function<std::wstring()> prefixCallback = nullptr;

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

#define LOG_RAW(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::RawLogger(), LOG_CTX, spdlog::level::trace, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_TIME(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::TimeLogger(), LOG_CTX, spdlog::level::trace, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))

#define LOG_INFO(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::Logger(), LOG_CTX, spdlog::level::info, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_DEBUG(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::Logger(), LOG_CTX, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_ERROR(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::Logger(), LOG_CTX, spdlog::level::err, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_WARNING(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::Logger(), LOG_CTX, spdlog::level::warn, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))

#define LOG_INFO_D(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::info, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_DEBUG_D(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_ERROR_D(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::err, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_WARNING_D(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::warn, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
//
// Use it inside static functions or with custom context:
#define LOG_INFO_S(_This, fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::info, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_DEBUG_S(_This, fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_ERROR_S(_This, fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::err, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_WARNING_S(_This, fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::warn, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))


#define LOG_FUNCTION_ENTER(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(lg::nullctx, lg::DefaultLoggers::FuncLogger(), LOG_CTX, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))

// TODO: solve problem with "std::string("~") + fmt" it is must be constexpr value
//#define LOG_FUNCTION_SCOPE(fmt, ...)                                                                                      \
//	LOG_FUNCTION_ENTER(EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__));                                                              \
//                                                                                                                             \
//	auto functionFinishLogScoped = H::MakeScope([&] {                                                                        \
//		LOG_FUNCTION_ENTER(std::string("~") + fmt); /* TODO: add suppoort for wstr */                                     \
//		});

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
            /*this->className##_fullClassNameA = H::WStrToStr(name);*/                                                            \
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