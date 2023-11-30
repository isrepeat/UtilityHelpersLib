#pragma once
// define these macros before first include spdlog headers
#define SPDLOG_WCHAR_TO_ANSI_SUPPORT
#define SPDLOG_WCHAR_FILENAMES
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include "spdlog/sinks/msvc_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#ifdef INSIDE_HELPERS_PROJECT
#include "Scope.h"
#include "Macros.h"
#include "String.hpp"
#include "Singleton.hpp"
#include "TypeTraits.hpp"
#else
#include <Helpers/Scope.h>
#include <Helpers/Macros.h>
#include <Helpers/String.hpp>
#include <Helpers/Singleton.hpp>
#include <Helpers/TypeTraits.hpp>
#endif
#include "CustomTypeSpecialization.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <array>


namespace lg {
    // define a "__classFullnameLogging" "member checker" class
    define_has_member(__ClassFullnameLogging);

   
    struct StandardLoggers {
        std::shared_ptr<spdlog::logger> logger;
        std::shared_ptr<spdlog::logger> rawLogger;
        std::shared_ptr<spdlog::logger> timeLogger;
        std::shared_ptr<spdlog::logger> funcLogger;
        std::shared_ptr<spdlog::logger> extendLogger;

        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSink;
        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSinkRaw;
        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSinkTime;
        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSinkFunc;
        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSinkExtend;

#ifdef _DEBUG
        std::shared_ptr<spdlog::logger> debugLogger;
#endif
    };


    // mb rename?
    class DefaultLoggers : public _Singleton<class DefaultLoggers> {
    private:
        using _MyBase = _Singleton<DefaultLoggers>;
        friend _MyBase; // to have access to private Ctor DefaultLoggers()

        DefaultLoggers();
    public:
        ~DefaultLoggers() = default;
        struct UnscopedData;

        static constexpr uintmax_t maxSizeLogFile = 1 * 1024 * 1024; // 1 MB (~ 10'000 rows)
        static constexpr size_t maxLoggers = 2;

        static void Init(const std::wstring& logFilePath, bool truncate = false, bool appendNewSessionMsg = true);
        static void InitForId(uint8_t loggerId, const std::wstring& logFilePath, bool truncate = false, bool appendNewSessionMsg = true);
        static std::string GetLastMessage();

        static std::shared_ptr<spdlog::logger> Logger(uint8_t id = 0);
        static std::shared_ptr<spdlog::logger> RawLogger(uint8_t id = 0);
        static std::shared_ptr<spdlog::logger> TimeLogger(uint8_t id = 0);
        static std::shared_ptr<spdlog::logger> FuncLogger(uint8_t id = 0);
        static std::shared_ptr<spdlog::logger> DebugLogger(uint8_t id = 0);
        static std::shared_ptr<spdlog::logger> ExtendLogger(uint8_t id = 0);


        // NOTE: overload for std::basic_string_view<T>
        //template<typename T, typename TClass, typename... Args>
        //static void Log_sv(
        //    TClass* classPtr,
        //    std::shared_ptr<spdlog::logger> logger,
        //    spdlog::source_loc location, spdlog::level::level_enum level, std::basic_string_view<T> formatSv, Args&&... args)
        //{
        //    Log<T, TClass, Args...>(classPtr, logger, location, level, formatSv, std::forward<Args>(args)...);
        //}

        template<typename T, typename TClass, typename... Args>
        static void Log(
            TClass* classPtr,
            std::shared_ptr<spdlog::logger> logger,
            spdlog::source_loc location, spdlog::level::level_enum level, fmt::basic_format_string<T, std::type_identity_t<Args>...> format, Args&&... args)
        {
            auto& _this = GetInstance();
            std::unique_lock lk{ _this.mxCustomFlagHandlers };

            if constexpr (has_member(std::remove_reference_t<decltype(std::declval<TClass>())>, __ClassFullnameLogging)) {
                _this.className = L" [" + classPtr->GetFullClassNameW() + L"]";
            }
            else {
                _this.className = L"";
            }
            (logger)->log(location, level, format, std::forward<Args&&>(args)...);
        }

    private:
        std::array<StandardLoggers, maxLoggers> standardLoggersList;
#ifdef _DEBUG
        const std::shared_ptr<spdlog::sinks::msvc_sink_mt> debugSink;
#endif

        std::mutex mxCustomFlagHandlers;
        std::string lastMessage; // spdlog converts all msg to char
        std::wstring className;
        std::function<std::wstring()> prefixCallback = nullptr;
        std::function<void(const std::string&)> postfixCallback = nullptr;

        std::shared_ptr<int> token = std::make_shared<int>();
    };

    constexpr H::nothing* nullctx = nullptr; // used to pass null ctx for logger explicilty
}

// TODO: find better solution to detect class context
H::nothing* __LgCtx(); // may be overwritten as class method that returned "this" (throught class fullname logging macro)


#if defined(LOG_CTX)
#error LOG_... macros already defined
#endif
#if defined(LOG_RAW) || defined(LOG_TIME)
#error LOG_... macros already defined
#endif
#if defined(LOG_DEBUG) || defined(LOG_ERROR) || defined(LOG_WARNING)
#error LOG_... macros already defined
#endif
#if defined(LOG_DEBUG_S) || defined(LOG_ERROR_S) || defined(LOG_WARNING_S)
#error LOG_... macros already defined
#endif
#if defined(LOG_DEBUG_D) || defined(LOG_ERROR_D) || defined(LOG_WARNING_D)
#error LOG_... macros already defined
#endif
#if defined(LOG_FUNCTION_ENTER_S) || defined(LOG_FUNCTION_ENTER_C) || defined(LOG_FUNCTION_ENTER)
#error LOG_... macros already defined
#endif
#if defined(LOG_FUNCTION_SCOPE_S) || defined(LOG_FUNCTION_SCOPE_C) || defined(LOG_FUNCTION_SCOPE)
#error LOG_... macros already defined
#endif


#if !defined(DISABLE_COMMON_LOGGING)
#define LOG_CTX spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}

#define LOG_RAW(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::RawLogger(), LOG_CTX, spdlog::level::trace, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_TIME(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::TimeLogger(), LOG_CTX, spdlog::level::trace, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))

#define LOG_DEBUG(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::Logger(), LOG_CTX, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_ERROR(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::Logger(), LOG_CTX, spdlog::level::err, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_WARNING(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::Logger(), LOG_CTX, spdlog::level::warn, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))

// Use it inside static functions or with custom context:
#define LOG_DEBUG_S(_This, fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_ERROR_S(_This, fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::err, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_WARNING_S(_This, fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, lg::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::warn, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))

#define LOG_DEBUG_D(fmt, ...) LOG_DEBUG_S(__LgCtx(), fmt, __VA_ARGS__)
#define LOG_ERROR_D(fmt, ...) LOG_ERROR_S(__LgCtx(), fmt, __VA_ARGS__)
#define LOG_WARNING_D(fmt, ...) LOG_WARNING_S(__LgCtx(), fmt, __VA_ARGS__)

// Extend logger save last message
#define LOG_DEBUG_EX(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::ExtendLogger(), LOG_CTX, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_ERROR_EX(fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::ExtendLogger(), LOG_CTX, spdlog::level::err, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))


#define LOG_FUNCTION_ENTER_S(_This, fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, lg::DefaultLoggers::FuncLogger(), LOG_CTX, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_FUNCTION_ENTER_C(fmt, ...) LOG_FUNCTION_ENTER_S(__LgCtx(), fmt, __VA_ARGS__)
#define LOG_FUNCTION_ENTER(fmt, ...) LOG_FUNCTION_ENTER_S(lg::nullctx, fmt, __VA_ARGS__)


#define LOG_FUNCTION_SCOPE_S(_This, fmt, ...)                                                                                                                                          \
    auto __fnCtx = LOG_CTX;                                                                                                                                                            \
	lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, lg::DefaultLoggers::FuncLogger(), __fnCtx, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__));                         \
                                                                                                                                                                                       \
	auto __functionFinishLogScoped = H::MakeScope([&] {                                                                                                                                \
		lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, lg::DefaultLoggers::FuncLogger(), __fnCtx, spdlog::level::debug, EXPAND_1_VA_ARGS_(JOIN_STRING("<= ", fmt), __VA_ARGS__)); \
		});

#define LOG_FUNCTION_SCOPE_C(fmt, ...) LOG_FUNCTION_SCOPE_S(__LgCtx(), fmt, __VA_ARGS__)
#define LOG_FUNCTION_SCOPE(fmt, ...) LOG_FUNCTION_SCOPE_S(lg::nullctx, fmt, __VA_ARGS__)

#else
#define LOG_CTX
#define LOG_RAW(fmt, ...)
#define LOG_TIME(fmt, ...)

#define LOG_DEBUG(fmt, ...)
#define LOG_ERROR(fmt, ...)
#define LOG_WARNING(fmt, ...)

#define LOG_DEBUG_S(_This, fmt, ...)
#define LOG_ERROR_S(_This, fmt, ...)
#define LOG_WARNING_S(_This, fmt, ...)

#define LOG_DEBUG_D(fmt, ...)
#define LOG_ERROR_D(fmt, ...)
#define LOG_WARNING_D(fmt, ...)

#define LOG_FUNCTION_ENTER_S(_This, fmt, ...)
#define LOG_FUNCTION_ENTER_C(fmt, ...)
#define LOG_FUNCTION_ENTER(fmt, ...)

#define LOG_FUNCTION_SCOPE_S(_This, fmt, ...)
#define LOG_FUNCTION_SCOPE_C(fmt, ...)
#define LOG_FUNCTION_SCOPE(fmt, ...)
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