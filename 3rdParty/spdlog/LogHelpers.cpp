#include "LogHelpers.h"
#include <Helpers/TokenSingleton.hpp>
#include <Helpers/FileSystem_inline.h>
#include <Helpers/Macros.h>
#include <set>


namespace lg {
    // Free letter flags: 'j', 'k', 'w' 

    class CustomMsgCallbackFlag : public spdlog::custom_flag_formatter {
    public:
        static const char flag = 'q';

        explicit CustomMsgCallbackFlag(std::function<std::wstring()> prefixFn, std::function<void(const std::string&)> postfixFn = nullptr)
            : prefixCallback{ prefixFn }
            , postfixCallback{ postfixFn }
        {
        }

        void format(const spdlog::details::log_msg& logMsg, const std::tm&, spdlog::memory_buf_t& dest) override {
            if (prefixCallback) {
                std::wstring prefix = std::wstring(padinfo_.width_, ' ') + prefixCallback();
                dest.append(prefix.data(), prefix.data() + prefix.size());
            }
            if (postfixCallback) {
                postfixCallback(std::string{ logMsg.payload.begin(), logMsg.payload.end() });
            }
        }

        std::unique_ptr<custom_flag_formatter> clone() const override {
            return spdlog::details::make_unique<CustomMsgCallbackFlag>(prefixCallback);
        }

    private:
        std::function<std::wstring()> prefixCallback;
        std::function<void(const std::string&)> postfixCallback;
    };


    enum class Pattern {
        Default,
        Extend,
        Debug,
        Func,
        Time,
        Raw,
    };

    // %l - log level
    // %t - thread id
    // %d.%m.%Y %H:%M:%S:%e - time
    // %q - optional prefix/postfix callback
    // %v - log msg
    const std::string GetPattern(Pattern value) {
        static const std::unordered_map<Pattern, std::string> patterns = {
            {Pattern::Default, "[%l] [%t] %d.%m.%Y %H:%M:%S:%e {%s:%# %!}%q %v"},
            {Pattern::Extend, "[%l] [%t] %d.%m.%Y %H:%M:%S:%e {%s:%# %!}%q %v"}, // use postfixCallback
            {Pattern::Debug, "[dbg] [%t] {%s:%# %!}%q %v"},
            {Pattern::Func, "[%l] [%t] %d.%m.%Y %H:%M:%S:%e {%s:%# %!}%q @@@ %v"},
            {Pattern::Time, "[%t] %d.%m.%Y %H:%M:%S:%e  %v"},
            {Pattern::Raw, "%v"},
        };
        return patterns.at(value);
    }



    struct DefaultLoggers::UnscopedData {
        UnscopedData()
            : defaultSink{ std::make_shared<spdlog::sinks::msvc_sink_mt>() }
            , defaultLogger{ std::make_shared<spdlog::logger>("default_logger", spdlog::sinks_init_list{ defaultSink }) }
        {
            auto formatterDebug = std::make_unique<spdlog::pattern_formatter>();
            formatterDebug->add_flag<CustomMsgCallbackFlag>(CustomMsgCallbackFlag::flag, nullptr).set_pattern(GetPattern(Pattern::Debug));
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
        formatterDebug->add_flag<CustomMsgCallbackFlag>(CustomMsgCallbackFlag::flag, prefixCallback).set_pattern(GetPattern(Pattern::Debug));
        debugSink->set_formatter(std::move(formatterDebug));
        debugSink->set_level(spdlog::level::trace);
#endif
        // DefaultLoggers::UnscopedData is created inside singleton
        TokenSingleton<DefaultLoggers>::SetToken(Passkey<DefaultLoggers>{}, this->token); 
    }



    void DefaultLoggers::Init(const std::wstring& logFilePath, bool truncate, bool appendNewSessionMsg) {
        GetInstance().InitForId(0, logFilePath, truncate, appendNewSessionMsg);
    }

    void DefaultLoggers::InitForId(uint8_t loggerId, const std::wstring& logFilePath, bool truncate, bool appendNewSessionMsg) {
        assertm(loggerId < maxLoggers, "loggerId out of bound");
        static std::set<std::wstring> initedLoggers;

        if (initedLoggers.count(logFilePath) > 0) {
            TimeLogger(loggerId)->warn("the logger on this path has already been initialized");
            return;
        }
        else {
            initedLoggers.insert(logFilePath);
        }

        if (!std::filesystem::exists(logFilePath)) {
            appendNewSessionMsg = false; // don't append new session message at first created log file
        }
        else {
            if (!truncate && std::filesystem::file_size(logFilePath) > maxSizeLogFile) {
                H::FS::RemoveBytesFromStart(logFilePath, maxSizeLogFile / 2, [](std::ofstream& file) {
                    std::string header = "... [truncated] \n\n";
                    file.write(header.data(), header.size());
                    });
            }
        }

        auto& _this = GetInstance();

        _this.standardLoggersList[loggerId].fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, truncate);
        auto formatterDefault = std::make_unique<spdlog::pattern_formatter>();
        formatterDefault->add_flag<CustomMsgCallbackFlag>(CustomMsgCallbackFlag::flag, _this.prefixCallback).set_pattern(GetPattern(Pattern::Default));
        _this.standardLoggersList[loggerId].fileSink->set_formatter(std::move(formatterDefault));
        _this.standardLoggersList[loggerId].fileSink->set_level(spdlog::level::trace);

        _this.standardLoggersList[loggerId].fileSinkRaw = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, truncate);
        _this.standardLoggersList[loggerId].fileSinkRaw->set_pattern(GetPattern(Pattern::Raw));
        _this.standardLoggersList[loggerId].fileSinkRaw->set_level(spdlog::level::trace);

        _this.standardLoggersList[loggerId].fileSinkTime = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, truncate);
        _this.standardLoggersList[loggerId].fileSinkTime->set_pattern(GetPattern(Pattern::Time));
        _this.standardLoggersList[loggerId].fileSinkTime->set_level(spdlog::level::trace);

        _this.standardLoggersList[loggerId].fileSinkFunc = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, truncate);
        auto formatterFunc = std::make_unique<spdlog::pattern_formatter>();
        formatterFunc->add_flag<CustomMsgCallbackFlag>(CustomMsgCallbackFlag::flag, _this.prefixCallback).set_pattern(GetPattern(Pattern::Func));
        _this.standardLoggersList[loggerId].fileSinkFunc->set_formatter(std::move(formatterFunc));
        _this.standardLoggersList[loggerId].fileSinkFunc->set_level(spdlog::level::trace);

        _this.standardLoggersList[loggerId].fileSinkExtend = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, truncate);
        auto formatterExtend = std::make_unique<spdlog::pattern_formatter>();
        formatterExtend->add_flag<CustomMsgCallbackFlag>(CustomMsgCallbackFlag::flag, _this.prefixCallback, _this.postfixCallback).set_pattern(GetPattern(Pattern::Default));
        _this.standardLoggersList[loggerId].fileSinkExtend->set_formatter(std::move(formatterExtend));
        _this.standardLoggersList[loggerId].fileSinkExtend->set_level(spdlog::level::trace);


        std::string nameId = loggerId > 0 ? "_" + std::to_string(loggerId) : "";

        _this.standardLoggersList[loggerId].logger = std::make_shared<spdlog::logger>("logger" + nameId, spdlog::sinks_init_list{ _this.standardLoggersList[loggerId].fileSink });
        _this.standardLoggersList[loggerId].logger->flush_on(spdlog::level::trace);
        _this.standardLoggersList[loggerId].logger->set_level(spdlog::level::trace);

        _this.standardLoggersList[loggerId].rawLogger = std::make_shared<spdlog::logger>("raw_logger" + nameId, spdlog::sinks_init_list{ _this.standardLoggersList[loggerId].fileSinkRaw });
        _this.standardLoggersList[loggerId].rawLogger->flush_on(spdlog::level::trace);
        _this.standardLoggersList[loggerId].rawLogger->set_level(spdlog::level::trace);

        _this.standardLoggersList[loggerId].timeLogger = std::make_shared<spdlog::logger>("time_logger" + nameId, spdlog::sinks_init_list{ _this.standardLoggersList[loggerId].fileSinkTime });
        _this.standardLoggersList[loggerId].timeLogger->flush_on(spdlog::level::trace);
        _this.standardLoggersList[loggerId].timeLogger->set_level(spdlog::level::trace);

        _this.standardLoggersList[loggerId].funcLogger = std::make_shared<spdlog::logger>("func_logger" + nameId, spdlog::sinks_init_list{ _this.standardLoggersList[loggerId].fileSinkFunc });
        _this.standardLoggersList[loggerId].funcLogger->flush_on(spdlog::level::trace);
        _this.standardLoggersList[loggerId].funcLogger->set_level(spdlog::level::trace);

#ifdef _DEBUG
        _this.standardLoggersList[loggerId].debugLogger = std::make_shared<spdlog::logger>("debug_logger" + nameId, spdlog::sinks_init_list{ _this.debugSink, _this.standardLoggersList[loggerId].fileSink });
        _this.standardLoggersList[loggerId].debugLogger->flush_on(spdlog::level::trace);
        _this.standardLoggersList[loggerId].debugLogger->set_level(spdlog::level::trace);
#endif

#ifdef _DEBUG
        _this.standardLoggersList[loggerId].extendLogger = std::make_shared<spdlog::logger>("extend_logger" + nameId, spdlog::sinks_init_list{ _this.debugSink, _this.standardLoggersList[loggerId].fileSinkExtend });
#else
        _this.standardLoggersList[loggerId].extendLogger = std::make_shared<spdlog::logger>("extend_logger" + nameId, spdlog::sinks_init_list{ _this.standardLoggersList[loggerId].fileSinkExtend });
#endif
        _this.standardLoggersList[loggerId].extendLogger->flush_on(spdlog::level::trace);
        _this.standardLoggersList[loggerId].extendLogger->set_level(spdlog::level::trace);


        if (appendNewSessionMsg) {
            _this.standardLoggersList[loggerId].rawLogger->info("\n");
            // whitespaces are selected by design
            _this.standardLoggersList[loggerId].rawLogger->info("==========================================================================================================");
            _this.standardLoggersList[loggerId].timeLogger->info("                       New session started");
            _this.standardLoggersList[loggerId].rawLogger->info("==========================================================================================================");
        }
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

H::nothing* __LgCtx() {
    return nullptr;
}