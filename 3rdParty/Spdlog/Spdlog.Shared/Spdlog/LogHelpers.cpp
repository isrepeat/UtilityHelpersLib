#include "LogHelpers.h"
#pragma message(PREPROCESSOR_MSG("Build LogHelpers.cpp with LOGGER_API = '" PP_STRINGIFY(LOGGER_API) "'"))
#include <Helpers/FileSystem_inline.h>
#include <Helpers/PackageProvider.h> // need link with Helpers.lib
#include <Helpers/TokenSingleton.hpp>
#include <Helpers/Macros.h>
#include <ComAPI/ComAPI.h>
#include <set>

namespace LOGGER_NS {
    // Free letter flags: 'G', 'h', 'J''j', 'K''k', 'N', 'Q', 'U', 'V', 'W''w', 'Z', '*', "?"

    // TODO: Use universal formatter to format source_location with paddings to make it more readable 
    class FunctionNameFormatter : public spdlog::custom_flag_formatter {
    public:
        static const char flag = '?';
        static const int maxFuncLenght = 30;

        explicit FunctionNameFormatter() = default;

        void format(const spdlog::details::log_msg& logMsg, const std::tm&, spdlog::memory_buf_t& dest) override {
            if (logMsg.source.empty()) {
                return;
            }

            // Make pretty function name
            //std::string_view svFuncName = logMsg.source.funcname;
            //if (svFuncName.size() > maxFuncLenght) {
            //    std::string_view svFuncNameStart = svFuncName.substr(0, maxFuncLenght / 2);
            //    std::string_view svFuncNameEnd = svFuncName.substr(svFuncName.size() - maxFuncLenght / 2);
            //    
            //    spdlog::details::fmt_helper::append_string_view({ svFuncNameStart.data(), svFuncNameStart.size() }, dest);
            //    spdlog::details::fmt_helper::append_string_view("...", dest);
            //    spdlog::details::fmt_helper::append_string_view({ svFuncNameEnd.data(), svFuncNameEnd.size() }, dest);
            //    return;
            //}

            spdlog::details::fmt_helper::append_string_view(logMsg.source.funcname, dest);
        }

        std::unique_ptr<custom_flag_formatter> clone() const override {
            return spdlog::details::make_unique<FunctionNameFormatter>();
        }
    };

    class MsgCallbackFormatter : public spdlog::custom_flag_formatter {
    public:
        static const char flag = 'q';

        explicit MsgCallbackFormatter(std::function<std::wstring()> prefixFn, std::function<void(const std::string&)> postfixFn = nullptr)
            : prefixCallback{ prefixFn }
            , postfixCallback{ postfixFn }
        {
        }

        void format(const spdlog::details::log_msg& logMsg, const std::tm&, spdlog::memory_buf_t& dest) override {
            switch (logMsg.level) {
            case spdlog::level::level_enum::err:
                spdlog::details::fmt_helper::append_string_view(" *** [E] =", dest);
                break;
            case spdlog::level::level_enum::warn:
                spdlog::details::fmt_helper::append_string_view(" *** [W] =", dest);
                break;
            }

            if (prefixCallback) {
                std::wstring prefix = std::wstring(padinfo_.width_, ' ') + prefixCallback();
                dest.append(prefix.data(), prefix.data() + prefix.size());
            }
            if (postfixCallback) {
                postfixCallback(std::string{ logMsg.payload.begin(), logMsg.payload.end() });
            }
        }

        std::unique_ptr<custom_flag_formatter> clone() const override {
            return spdlog::details::make_unique<MsgCallbackFormatter>(prefixCallback, postfixCallback);
        }

    private:
        std::function<std::wstring()> prefixCallback;
        std::function<void(const std::string&)> postfixCallback;
    };


    enum class Pattern {
        Default,
        Raw,
        Time,
        Func,
        Extend,
        Debug,
        DebugFn,
    };

    // %l - log level
    // %t - thread id
    // %d.%m.%Y %H:%M:%S:%e - time
    // %q - optional prefix/postfix callback
    // %v - log msg
    // %^/%$ - start/end color range
    const std::string GetPattern(Pattern value) {
        static const std::unordered_map<Pattern, std::string> patterns = {
            {Pattern::Default, "[%L] [%t] %d.%m.%Y %H:%M:%S:%e {%s:%# %?}%q %v"},
            {Pattern::Raw, "%v"},
            {Pattern::Time, "[%L] %d.%m.%Y %H:%M:%S:%e  %v"},
            {Pattern::Func, "[%L] [%t] %d.%m.%Y %H:%M:%S:%e {%s:%# %?}%q @@@ %v"},
            {Pattern::Extend, "[%L] [%t] %d.%m.%Y %H:%M:%S:%e {%s:%# %?}%q %v"}, // use postfixCallback
            {Pattern::Debug, "[%L] [%t] %H:%M:%S:%e {%s:%# %?}%^%q %v%$"},
            {Pattern::DebugFn, "[%L] [%t] %H:%M:%S:%e {%s:%# %?}%^%q @@@ %v%$"},
        };
        return patterns.at(value);
    }



    struct DefaultLoggers::UnscopedData {
        UnscopedData()
            : defaultSink{ std::make_shared<spdlog::sinks::msvc_sink_mt>() }
            , defaultLogger{ std::make_shared<spdlog::logger>("default_logger", spdlog::sinks_init_list{ defaultSink }) }
        {
            auto formatterDebug = std::make_unique<spdlog::pattern_formatter>();
            formatterDebug->add_flag<FunctionNameFormatter>(FunctionNameFormatter::flag).set_pattern(GetPattern(Pattern::Debug));
            formatterDebug->add_flag<MsgCallbackFormatter>(MsgCallbackFormatter::flag, nullptr).set_pattern(GetPattern(Pattern::Debug));
            defaultSink->set_formatter(std::move(formatterDebug));
            defaultSink->set_level(spdlog::level::trace);

            defaultLogger->flush_on(spdlog::level::trace);
            defaultLogger->set_level(spdlog::level::trace);
        }
        ~UnscopedData() {
            assertm(false, "Unexpected destructor call");
        }

        const std::shared_ptr<spdlog::logger> DefaultLogger() const {
            return defaultLogger;
        }

    private:
        const std::shared_ptr<spdlog::sinks::msvc_sink_mt> defaultSink;
        const std::shared_ptr<spdlog::logger> defaultLogger;
    };



    DefaultLoggers::DefaultLoggers()
#ifdef _DEBUG
        : debugSink{ std::make_shared<spdlog::sinks::msvc_sink_mt>() }
#endif
    {
        prefixCallback = [this] {
            return className;
        };
        postfixCallback = [this](const std::string& logMsg) {
            lastMessage = logMsg;
        };

#ifdef _DEBUG
        auto formatterDebug = std::make_unique<spdlog::pattern_formatter>();
        formatterDebug->add_flag<FunctionNameFormatter>(FunctionNameFormatter::flag).set_pattern(GetPattern(Pattern::Debug));
        formatterDebug->add_flag<MsgCallbackFormatter>(MsgCallbackFormatter::flag, prefixCallback).set_pattern(GetPattern(Pattern::Debug));
        debugSink->set_formatter(std::move(formatterDebug));
        debugSink->set_level(spdlog::level::trace);
#endif
        // DefaultLoggers::UnscopedData is created inside singleton
        TokenSingleton<DefaultLoggers>::SetToken(Passkey<DefaultLoggers>{}, this->token);
    }

    DefaultLoggers::~DefaultLoggers() {
        standardLoggersList = {}; // clear loggers to release sinks
    }



    void DefaultLoggers::Init(std::filesystem::path logFilePath, HELPERS_NS::Flags<InitFlags> initFlags) {
        GetInstance().InitForId(0, logFilePath, initFlags);
    }

    void DefaultLoggers::InitForId(uint8_t loggerId, std::filesystem::path logFilePath, HELPERS_NS::Flags<InitFlags> initFlags) {
        assertm(loggerId < maxLoggers, "loggerId out of bound");
        // TODO: Use H::Bimap<"loggerPath", loggerId> and move to class member (combine with initializedLoggersById)
        static std::set<std::wstring> initializedLoggersByPath;

        if (initFlags.Has(InitFlags::CreateInPackageFolder) && HELPERS_NS::PackageProvider::IsRunningUnderPackage()) {
            logFilePath = ComApi::GetPackageFolder() / logFilePath.relative_path();
        }

        if (initializedLoggersByPath.count(logFilePath) > 0) {
            TimeLogger(loggerId)->warn("the logger on this path has already been initialized");
            return;
        }
        else {
            initializedLoggersByPath.insert(logFilePath);
        }

        if (!std::filesystem::exists(logFilePath)) {
            initFlags &= ~InitFlags::AppendNewSessionMsg; // don't append new session message at first created log file
            std::filesystem::create_directories(logFilePath.parent_path());
        }
        else {
            auto logSize = std::filesystem::file_size(logFilePath);
            if (!initFlags.Has(InitFlags::Truncate) && logSize > maxSizeLogFile) {
                auto truncatedBytes = logSize - maxSizeLogFile / 2;
                HELPERS_NS::FS::RemoveBytesFromStart(logFilePath, truncatedBytes, [](std::ofstream& file) {
                    std::string header = "... [truncated] \n\n";
                    file.write(header.data(), header.size());
                    });
            }
        }


        auto& _this = GetInstance();
        if (_this.initializedLoggersById.count(loggerId) > 0) {
            TimeLogger(loggerId)->warn("the logger on this id = {} has already been initialized, continue reinitalize ...", loggerId);
        }
        else {
            _this.initializedLoggersById.insert(loggerId);
        }

        _this.standardLoggersList[loggerId].fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, initFlags.Has(InitFlags::Truncate));
        auto formatterDefault = std::make_unique<spdlog::pattern_formatter>();
        formatterDefault->add_flag<FunctionNameFormatter>(FunctionNameFormatter::flag).set_pattern(GetPattern(Pattern::Default));
        formatterDefault->add_flag<MsgCallbackFormatter>(MsgCallbackFormatter::flag, _this.prefixCallback).set_pattern(GetPattern(Pattern::Default));
        _this.standardLoggersList[loggerId].fileSink->set_formatter(std::move(formatterDefault));
        _this.standardLoggersList[loggerId].fileSink->set_level(spdlog::level::trace);

        _this.standardLoggersList[loggerId].fileSinkRaw = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, initFlags.Has(InitFlags::Truncate));
        if (initFlags.Has(InitFlags::DisableEOLforRawLogger)) {
            auto formatterRaw = std::make_unique<spdlog::pattern_formatter>(GetPattern(Pattern::Raw), spdlog::pattern_time_type::local, std::string(""));
            _this.standardLoggersList[loggerId].fileSinkRaw->set_formatter(std::move(formatterRaw));
        }
        else {
            _this.standardLoggersList[loggerId].fileSinkRaw->set_pattern(GetPattern(Pattern::Raw));
        }
        _this.standardLoggersList[loggerId].fileSinkRaw->set_level(spdlog::level::trace);


        _this.standardLoggersList[loggerId].fileSinkTime = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, initFlags.Has(InitFlags::Truncate));
        _this.standardLoggersList[loggerId].fileSinkTime->set_pattern(GetPattern(Pattern::Time));
        _this.standardLoggersList[loggerId].fileSinkTime->set_level(spdlog::level::trace);

        _this.standardLoggersList[loggerId].fileSinkFunc = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, initFlags.Has(InitFlags::Truncate));
        auto formatterFunc = std::make_unique<spdlog::pattern_formatter>();
        formatterFunc->add_flag<FunctionNameFormatter>(FunctionNameFormatter::flag).set_pattern(GetPattern(Pattern::Func));
        formatterFunc->add_flag<MsgCallbackFormatter>(MsgCallbackFormatter::flag, _this.prefixCallback).set_pattern(GetPattern(Pattern::Func));
        _this.standardLoggersList[loggerId].fileSinkFunc->set_formatter(std::move(formatterFunc));
        _this.standardLoggersList[loggerId].fileSinkFunc->set_level(spdlog::level::trace);

        _this.standardLoggersList[loggerId].fileSinkExtend = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, initFlags.Has(InitFlags::Truncate));
        auto formatterExtend = std::make_unique<spdlog::pattern_formatter>();
        formatterExtend->add_flag<FunctionNameFormatter>(FunctionNameFormatter::flag).set_pattern(GetPattern(Pattern::Extend));
        formatterExtend->add_flag<MsgCallbackFormatter>(MsgCallbackFormatter::flag, _this.prefixCallback, _this.postfixCallback).set_pattern(GetPattern(Pattern::Extend));
        _this.standardLoggersList[loggerId].fileSinkExtend->set_formatter(std::move(formatterExtend));
        _this.standardLoggersList[loggerId].fileSinkExtend->set_level(spdlog::level::trace);

        if (initFlags.Has(InitFlags::EnableLogToStdout)) {
            _this.stdoutDebugColorSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            auto formatterDebug = std::make_unique<spdlog::pattern_formatter>();
            formatterDebug->add_flag<FunctionNameFormatter>(FunctionNameFormatter::flag).set_pattern(GetPattern(Pattern::Debug));
            formatterDebug->add_flag<MsgCallbackFormatter>(MsgCallbackFormatter::flag, _this.prefixCallback).set_pattern(GetPattern(Pattern::Debug));
            _this.stdoutDebugColorSink->set_formatter(std::move(formatterDebug));
            _this.stdoutDebugColorSink->set_color(spdlog::level::err, FOREGROUND_RED);
            _this.stdoutDebugColorSink->set_color(spdlog::level::warn, FOREGROUND_RED);
            _this.stdoutDebugColorSink->set_color(spdlog::level::debug, FOREGROUND_INTENSITY); // dark-gray
            _this.stdoutDebugColorSink->set_level(spdlog::level::trace);

            _this.stdoutDebugFnColorSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            auto formatterFunc = std::make_unique<spdlog::pattern_formatter>();
            formatterFunc->add_flag<FunctionNameFormatter>(FunctionNameFormatter::flag).set_pattern(GetPattern(Pattern::DebugFn));
            formatterFunc->add_flag<MsgCallbackFormatter>(MsgCallbackFormatter::flag, _this.prefixCallback).set_pattern(GetPattern(Pattern::DebugFn));
            _this.stdoutDebugFnColorSink->set_formatter(std::move(formatterFunc));
            _this.stdoutDebugFnColorSink->set_color(spdlog::level::debug, FOREGROUND_GREEN | FOREGROUND_BLUE);
            _this.stdoutDebugFnColorSink->set_level(spdlog::level::trace);
        }


        spdlog::sinks_init_list loggerSinks = { _this.standardLoggersList[loggerId].fileSink };
        spdlog::sinks_init_list rawLoggerSinks = { _this.standardLoggersList[loggerId].fileSinkRaw };
        spdlog::sinks_init_list timeLoggerSinks = { _this.standardLoggersList[loggerId].fileSinkTime };
        spdlog::sinks_init_list funcLoggerSinks = { _this.standardLoggersList[loggerId].fileSinkFunc };
        spdlog::sinks_init_list extendLoggerSinks = { _this.standardLoggersList[loggerId].fileSinkExtend };

        if (initFlags.Has(InitFlags::EnableLogToStdout)) {
            loggerSinks = { _this.stdoutDebugColorSink, _this.standardLoggersList[loggerId].fileSink };
            funcLoggerSinks = { _this.stdoutDebugFnColorSink, _this.standardLoggersList[loggerId].fileSinkFunc };

            if (initFlags.Has(InitFlags::RedirectRawTimeLogToStdout)) {
                rawLoggerSinks = { _this.stdoutDebugColorSink, _this.standardLoggersList[loggerId].fileSinkRaw };
                timeLoggerSinks = { _this.stdoutDebugColorSink, _this.standardLoggersList[loggerId].fileSinkTime };
            }
        }

#ifdef _DEBUG
        extendLoggerSinks = { _this.debugSink, _this.standardLoggersList[loggerId].fileSinkExtend };

        spdlog::sinks_init_list debugLoggerSinks = { _this.debugSink, _this.standardLoggersList[loggerId].fileSink };
        if (initFlags.Has(InitFlags::EnableLogToStdout)) {
            debugLoggerSinks = { _this.debugSink, _this.stdoutDebugColorSink, _this.standardLoggersList[loggerId].fileSink };
        }
#endif

        std::string nameId = loggerId > 0 ? "_" + std::to_string(loggerId) : "";

        _this.standardLoggersList[loggerId].logger = std::make_shared<spdlog::logger>("logger" + nameId, loggerSinks);
        _this.standardLoggersList[loggerId].logger->flush_on(spdlog::level::trace);
        _this.standardLoggersList[loggerId].logger->set_level(spdlog::level::debug);

        _this.standardLoggersList[loggerId].rawLogger = std::make_shared<spdlog::logger>("raw_logger" + nameId, rawLoggerSinks);
        _this.standardLoggersList[loggerId].rawLogger->flush_on(spdlog::level::trace);
        _this.standardLoggersList[loggerId].rawLogger->set_level(spdlog::level::debug);

        _this.standardLoggersList[loggerId].timeLogger = std::make_shared<spdlog::logger>("time_logger" + nameId, timeLoggerSinks);
        _this.standardLoggersList[loggerId].timeLogger->flush_on(spdlog::level::trace);
        _this.standardLoggersList[loggerId].timeLogger->set_level(spdlog::level::debug);

        _this.standardLoggersList[loggerId].funcLogger = std::make_shared<spdlog::logger>("func_logger" + nameId, funcLoggerSinks);
        _this.standardLoggersList[loggerId].funcLogger->flush_on(spdlog::level::trace);
        _this.standardLoggersList[loggerId].funcLogger->set_level(spdlog::level::debug);

        _this.standardLoggersList[loggerId].extendLogger = std::make_shared<spdlog::logger>("extend_logger" + nameId, extendLoggerSinks);
        _this.standardLoggersList[loggerId].extendLogger->flush_on(spdlog::level::trace);
        _this.standardLoggersList[loggerId].extendLogger->set_level(spdlog::level::debug);

#ifdef _DEBUG
        _this.standardLoggersList[loggerId].debugLogger = std::make_shared<spdlog::logger>("debug_logger" + nameId, debugLoggerSinks);
        _this.standardLoggersList[loggerId].debugLogger->flush_on(spdlog::level::trace);
        _this.standardLoggersList[loggerId].debugLogger->set_level(spdlog::level::debug);
#endif

        if (initFlags.Has(InitFlags::AppendNewSessionMsg)) {
            std::string rawEOL = "";
            if (initFlags.Has(InitFlags::DisableEOLforRawLogger)) {
                rawEOL = "\n";
            }
            // whitespaces are selected by design
            _this.standardLoggersList[loggerId].rawLogger->debug("\n" + rawEOL);
            _this.standardLoggersList[loggerId].rawLogger->debug("==========================================================================================================" + rawEOL);
            _this.standardLoggersList[loggerId].timeLogger->debug("                       New session started");
            _this.standardLoggersList[loggerId].rawLogger->debug("==========================================================================================================" + rawEOL);
        }
    }

    bool DefaultLoggers::IsInitialized(uint8_t id) {
        return GetInstance().initializedLoggersById.count(id) > 0;
    }

    std::string DefaultLoggers::GetLastMessage() {
        std::unique_lock lk{ GetInstance().mxCustomFlagHandlers };
        return GetInstance().lastMessage;
    }


    std::shared_ptr<spdlog::logger> DefaultLoggers::Logger(uint8_t id) {
        auto& _this = GetInstance(); // ensure that token set

        if (TokenSingleton<DefaultLoggers>::IsExpired() || !_this.standardLoggersList[id].logger) {
            return TokenSingleton<DefaultLoggers>::GetData().DefaultLogger();
        }
        return GetInstance().standardLoggersList[id].logger;
    }

    std::shared_ptr<spdlog::logger> DefaultLoggers::RawLogger(uint8_t id) {
        auto& _this = GetInstance();

        if (TokenSingleton<DefaultLoggers>::IsExpired() || !_this.standardLoggersList[id].rawLogger) {
            return TokenSingleton<DefaultLoggers>::GetData().DefaultLogger();
        }
        return GetInstance().standardLoggersList[id].rawLogger;
    }

    std::shared_ptr<spdlog::logger> DefaultLoggers::TimeLogger(uint8_t id) {
        auto& _this = GetInstance();

        if (TokenSingleton<DefaultLoggers>::IsExpired() || !_this.standardLoggersList[id].timeLogger) {
            return TokenSingleton<DefaultLoggers>::GetData().DefaultLogger();
        }
        return GetInstance().standardLoggersList[id].timeLogger;
    }

    std::shared_ptr<spdlog::logger> DefaultLoggers::FuncLogger(uint8_t id) {
        auto& _this = GetInstance();

        if (TokenSingleton<DefaultLoggers>::IsExpired() || !_this.standardLoggersList[id].funcLogger) {
            return TokenSingleton<DefaultLoggers>::GetData().DefaultLogger();
        }
        return GetInstance().standardLoggersList[id].funcLogger;
    }

    std::shared_ptr<spdlog::logger> DefaultLoggers::DebugLogger(uint8_t id) {
#ifdef _DEBUG
        auto& _this = GetInstance();

        if (TokenSingleton<DefaultLoggers>::IsExpired() || !_this.standardLoggersList[id].debugLogger) {
            return TokenSingleton<DefaultLoggers>::GetData().DefaultLogger();
        }
        return GetInstance().standardLoggersList[id].debugLogger;
#else
        return Logger(id);
#endif
    }

    std::shared_ptr<spdlog::logger> DefaultLoggers::ExtendLogger(uint8_t id) {
        auto& _this = GetInstance();

        if (TokenSingleton<DefaultLoggers>::IsExpired() || !_this.standardLoggersList[id].extendLogger) {
            return TokenSingleton<DefaultLoggers>::GetData().DefaultLogger();
        }
        return GetInstance().standardLoggersList[id].extendLogger;
    }
}

LOGGER_API HELPERS_NS::nothing* __LgCtx() {
    return nullptr;
}