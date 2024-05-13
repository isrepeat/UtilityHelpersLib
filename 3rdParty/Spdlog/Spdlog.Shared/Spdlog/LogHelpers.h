#pragma once
#include <Preprocessor.h>
#pragma message(PREPROCESSOR_FILE_INCLUDED("LogHelpers.h"))
//#pragma message("include 'LogHelpers.h' [begin]")

// You can compile static library with "dllexport" to export symbols through dll.
// WARNING: Client must use "dllimport" if this project compiled as dll otherwise will be problems, for example:
//          - DefaultLoggers singleton may exist in two instances.
// NOTE: This macro must be redefined at global level (in <PreprocessorDefinitions>)
#ifndef LOGGER_API
#define LOGGER_API
#else
#pragma message(PREPROCESSOR_MSG("LOGGER_API already defined = '" PP_STRINGIFY(LOGGER_API) "'"))
#endif

#ifndef LOGGER_NS_ALIAS
#define LOGGER_NS_ALIAS lg
#else
#pragma message(PREPROCESSOR_MSG("LOGGER_NS_ALIAS already defined = '" PP_STRINGIFY(LOGGER_NS_ALIAS) "'"))
#endif

#define LOGGER_NS __lg_ns
namespace LOGGER_NS {} // create uniq "logger namespace" for this project
namespace LOGGER_NS_ALIAS = LOGGER_NS; // set your alias for original "logger namespace" (defined via macro)

// define these macros before first include spdlog headers
#define SPDLOG_WCHAR_TO_ANSI_SUPPORT
#define SPDLOG_WCHAR_FILENAMES


#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/callback_sink.h>
//#pragma message("include 'LogHelpers.h' [spdlog files included]")

//
// Define macros to make ability use it in custom includes before DefaultLoggers defined.
// TODO: try use forward declaration for DefaultLoggers or rewrite include dependencies.
//
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

#define LOG_RAW(fmt, ...) LOGGER_NS::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), LOGGER_NS::DefaultLoggers::RawLogger(), LOG_CTX, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_TIME(fmt, ...) LOGGER_NS::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), LOGGER_NS::DefaultLoggers::TimeLogger(), LOG_CTX, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))

#define LOG_DEBUG(fmt, ...) LOGGER_NS::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), LOGGER_NS::DefaultLoggers::Logger(), LOG_CTX, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_ERROR(fmt, ...) LOGGER_NS::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), LOGGER_NS::DefaultLoggers::Logger(), LOG_CTX, spdlog::level::err, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_WARNING(fmt, ...) LOGGER_NS::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), LOGGER_NS::DefaultLoggers::Logger(), LOG_CTX, spdlog::level::warn, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))

// Use it inside static functions or with custom context:
#define LOG_DEBUG_S(_This, fmt, ...) LOGGER_NS::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, LOGGER_NS::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_ERROR_S(_This, fmt, ...) LOGGER_NS::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, LOGGER_NS::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::err, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_WARNING_S(_This, fmt, ...) LOGGER_NS::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, LOGGER_NS::DefaultLoggers::DebugLogger(), LOG_CTX, spdlog::level::warn, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))

#define LOG_DEBUG_D(fmt, ...) LOG_DEBUG_S(__LgCtx(), fmt, __VA_ARGS__)
#define LOG_ERROR_D(fmt, ...) LOG_ERROR_S(__LgCtx(), fmt, __VA_ARGS__)
#define LOG_WARNING_D(fmt, ...) LOG_WARNING_S(__LgCtx(), fmt, __VA_ARGS__)

// Extend logger save last message
#define LOG_DEBUG_EX(fmt, ...) LOGGER_NS::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), LOGGER_NS::DefaultLoggers::ExtendLogger(), LOG_CTX, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_ERROR_EX(fmt, ...) LOGGER_NS::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), LOGGER_NS::DefaultLoggers::ExtendLogger(), LOG_CTX, spdlog::level::err, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))


#define LOG_FUNCTION_ENTER_S(_This, fmt, ...) LOGGER_NS::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, LOGGER_NS::DefaultLoggers::FuncLogger(), LOG_CTX, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_FUNCTION_ENTER_C(fmt, ...) LOG_FUNCTION_ENTER_S(__LgCtx(), fmt, __VA_ARGS__)
#define LOG_FUNCTION_ENTER(fmt, ...) LOG_FUNCTION_ENTER_S(LOGGER_NS::nullctx, fmt, __VA_ARGS__)


#define LOG_SCOPED(__LOGGER__, _This, fmt, ...)                                                                                                                          \
    auto __fnCtx = LOG_CTX;                                                                                                                                              \
    LOGGER_NS::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, __LOGGER__, __fnCtx, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__));                          \
                                                                                                                                                                         \
    auto __functionFinishLogScoped = HELPERS_NS::MakeScope([&] {                                                                                                         \
        LOGGER_NS::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, __LOGGER__, __fnCtx, spdlog::level::debug, EXPAND_1_VA_ARGS_(JOIN_STRING("<= ", fmt), __VA_ARGS__));  \
        });


#define LOG_DEBUG_SCOPE_D(fmt, ...)  LOG_SCOPED(LOGGER_NS::DefaultLoggers::DebugLogger(), __LgCtx(), fmt, __VA_ARGS__)


#define LOG_FUNCTION_SCOPE_S(_This, fmt, ...)                                                                                                                                                         \
    auto __fnCtx = LOG_CTX;                                                                                                                                                                           \
	LOGGER_NS::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, LOGGER_NS::DefaultLoggers::FuncLogger(), __fnCtx, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__));                          \
                                                                                                                                                                                                      \
	auto __functionFinishLogScoped = HELPERS_NS::MakeScope([&] {                                                                                                                                      \
		LOGGER_NS::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(_This, LOGGER_NS::DefaultLoggers::FuncLogger(), __fnCtx, spdlog::level::debug, EXPAND_1_VA_ARGS_(JOIN_STRING("<= ", fmt), __VA_ARGS__));  \
		});

#define LOG_FUNCTION_SCOPE_C(fmt, ...) LOG_FUNCTION_SCOPE_S(__LgCtx(), fmt, __VA_ARGS__)
#define LOG_FUNCTION_SCOPE(fmt, ...) LOG_FUNCTION_SCOPE_S(LOGGER_NS::nullctx, fmt, __VA_ARGS__)

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
// Declare at the top of a class
#define CLASS_FULLNAME_LOGGING_INLINE_IMPLEMENTATION(className)                                                               \
	private:                                                                                                                  \
		std::string className##_fullClassNameA = ""#className;                                                                \
		std::wstring className##_fullClassNameW = L""#className;                                                              \
                                                                                                                              \
	public:                                                                                                                   \
	    void __ClassFullnameLogging() {}                                                                                      \
	                                                                                                                          \
	    const className* __LgCtx() const {                                                                                    \
            return this;                                                                                                      \
        }                                                                                                                     \
	                                                                                                                          \
        void SetFullClassName(std::wstring name) {                                                                            \
            LOG_DEBUG_D(L"Full class name = {}", name);                                                                       \
            this->SetFullClassNameSilent(name);                                                                               \
        }                                                                                                                     \
	                                                                                                                          \
        void SetFullClassNameSilent(std::wstring name) {                                                                      \
            this->className##_fullClassNameA = HELPERS_NS::WStrToStr(name);                                                   \
            this->className##_fullClassNameW = name;                                                                          \
        }                                                                                                                     \
                                                                                                                              \
		const std::string& GetFullClassNameA() const {                                                                        \
			return this->className##_fullClassNameA;                                                                          \
		}                                                                                                                     \
                                                                                                                              \
		const std::wstring& GetFullClassNameW() const {                                                                       \
			return this->className##_fullClassNameW;                                                                          \
		}                                                                                                                     \
                                                                                                                              \
        static const std::wstring GetOriginalClassName() {                                                                    \
			static std::wstring originalClassName = L""#className;                                                            \
			return originalClassName;                                                                                         \
		}                                                                                                                     \
	                                                                                                                          \
	private: // back default class modifier

#else
#define CLASS_FULLNAME_LOGGING_INLINE_IMPLEMENTATION(className)
#endif



#define USE_DYNAMIC_SINK 0
#include <Helpers/TypeTraits.hpp>
#include <Helpers/Singleton.hpp>
#include <Helpers/String.hpp>
#include <Helpers/Macros.h>
#include <Helpers/Scope.h>
#include <Helpers/Flags.h>

#if USE_DYNAMIC_SINK
// TODO: use forward declaration for DefaultLoggers::Log() static method before LOG_... macros definition above
//       because Time.h include Rational.h and last one use LOG_... macros with undefined DefaultLoggers class yet
#include <Helpers/Semaphore.h>
#include <Helpers/Time.h>
#include "DynamicFileSink.h"
#endif
#include "CustomTypeSpecialization.h"
//#pragma message("include 'LogHelpers.h' [helpers files included]")

#include <unordered_map>
#include <filesystem>
#include <string>
#include <memory>
#include <array>
#include <set>


namespace LOGGER_NS {
    // define a "__classFullnameLogging" "member checker" class
    define_has_member(__ClassFullnameLogging);
   
    struct LOGGER_API StandardLoggers {
        std::shared_ptr<spdlog::logger> logger;
        std::shared_ptr<spdlog::logger> rawLogger;
        std::shared_ptr<spdlog::logger> timeLogger;
        std::shared_ptr<spdlog::logger> funcLogger;
        std::shared_ptr<spdlog::logger> extendLogger;
#ifdef _DEBUG
        std::shared_ptr<spdlog::logger> debugLogger;
#endif

#if USE_DYNAMIC_SINK
        std::shared_ptr<DynamicFileSinkMt> fileSink;
        std::shared_ptr<DynamicFileSinkMt> fileSinkRaw;
        std::shared_ptr<DynamicFileSinkMt> fileSinkTime;
        std::shared_ptr<DynamicFileSinkMt> fileSinkFunc;
        std::shared_ptr<DynamicFileSinkMt> fileSinkExtend;

        std::shared_ptr<HELPERS_NS::Timer> logSizeLimitChecker;
        std::shared_ptr<HELPERS_NS::EventObject> pauseLoggingEvent;
#else
        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSink;
        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSinkRaw;
        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSinkTime;
        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSinkFunc;
        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSinkExtend;
#endif
    };


    enum InitFlags { // declare without 'class' to simplify flags concatenation
        None = 0x00,
        Truncate = 0x01,
        AppendNewSessionMsg = 0x02,
        CreateInPackageFolder = 0x04,
        EnableLogToStdout = 0x08,
        RedirectRawTimeLogToStdout = 0x10,
        DisableEOLforRawLogger = 0x20, // not append '\n' at the end of line

        DefaultFlags = AppendNewSessionMsg
    };

    // mb rename?
    class LOGGER_API DefaultLoggers : public _Singleton<class DefaultLoggers> {
    private:
        using _MyBase = _Singleton<DefaultLoggers>;
        friend _MyBase; // to have access to private Ctor DefaultLoggers()

        DefaultLoggers();
    public:
        ~DefaultLoggers();

        struct UnscopedData;

        static constexpr uintmax_t maxSizeLogFile = 5 * 1024 * 1024; // 5 MB (~ 50'000 rows)
        static constexpr std::chrono::milliseconds logSizeCheckInterval{30'000};
        static constexpr size_t maxLoggers = 2;

        static void Init(std::filesystem::path logFilePath, HELPERS_NS::Flags<InitFlags> initFlags = InitFlags::DefaultFlags);
        static void InitForId(uint8_t loggerId, std::filesystem::path logFilePath, HELPERS_NS::Flags<InitFlags> initFlags = InitFlags::DefaultFlags);
        static bool IsInitialized(uint8_t id = 0);
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
                _this.className = L" ["
                    + (classPtr ? classPtr->GetFullClassNameW() : TClass::GetOriginalClassName() + L"(nullptr)")
                    + L"]";
            }
            else {
                _this.className = L"";
            }
            logger->log(location, level, format, std::forward<Args&&>(args)...);
        }

    private:
#if USE_DYNAMIC_SINK
        static void CheckLogFileSize(StandardLoggers& loggers);
#endif

        //const std::unordered_map<uint8_t, bool> initializedLoggersById;
        std::set<uint8_t> initializedLoggersById;
#ifdef _DEBUG
        const std::shared_ptr<spdlog::sinks::msvc_sink_mt> debugSink;
#endif
        std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> stdoutDebugColorSink;
        std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> stdoutDebugFnColorSink;

        std::array<StandardLoggers, maxLoggers> standardLoggersList;

        std::mutex mxCustomFlagHandlers;
        std::string lastMessage; // spdlog converts all msg to char
        std::wstring className;
        std::function<std::wstring()> prefixCallback = nullptr;
        std::function<void(const std::string&)> postfixCallback = nullptr;

        std::shared_ptr<int> token = std::make_shared<int>();

#if USE_DYNAMIC_SINK
        HELPERS_NS::Semaphore logSizeCheckSem;
#endif

        static constexpr std::string_view logTruncationMessage{"... [truncated] \n\n"};
    };

    constexpr HELPERS_NS::nothing* nullctx = nullptr; // used to pass null ctx for logger explicilty
}

// TODO: find better solution to detect class context
LOGGER_API HELPERS_NS::nothing* __LgCtx(); // may be overwritten as class method that returned "this" (throught class fullname logging macro)

//#pragma message("include 'LogHelpers.h' [end]")