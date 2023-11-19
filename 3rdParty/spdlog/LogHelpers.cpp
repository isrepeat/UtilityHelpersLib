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

        explicit CustomMsgCallbackFlag(std::function<const std::wstring& ()> prefixFn, std::function<void(const std::string&)> postfixFn = nullptr)
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
        std::function<void(std::string)> postfixCallback;
    };



    constexpr uintmax_t maxSizeLogFile = 1 * 1024 * 1024; // 1 MB (~ 10'000 rows)

    enum class Pattern {
        Default,
        Extend,
        Debug,
        Error,
        Func,
        Time,
        Raw,
    };

    // %v - log_msg
    // %q - optional prefix/postfix callback
    const std::string GetPattern(Pattern value) {
        static const std::unordered_map<Pattern, std::string> patterns = {
            {Pattern::Default, "[%l] [%t] %d.%m.%Y %H:%M:%S:%e {%s:%# %!}%q %v"},
            {Pattern::Extend, "[%l] [%t] %d.%m.%Y %H:%M:%S:%e {%s:%# %!}%q %v"}, // use postfixCallback
            {Pattern::Debug, "[dbg] [%t] {%s:%# %!}%q %v"},
            {Pattern::Func, "[%l] [%t] %d.%m.%Y %H:%M:%S:%e {%s:%# %!}%q @@@ %v"},
            {Pattern::Time, "%d.%m.%Y %H:%M:%S:%e  %v"},
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
        : prefixCallback{ [this] {
            return className;
            }}
        , postfixCallback{ [this](const std::string& logMsg) {
                lastMessage = logMsg;
            }}
#ifdef _DEBUG
        , debugSink{ std::make_shared<spdlog::sinks::msvc_sink_mt>() }
#endif
    {
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
        static std::set<std::wstring> initedLoggers;
        if (initedLoggers.count(logFilePath) > 0) {
            TimeLogger()->warn("the logger on this path has already been initialized");
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

        _this.fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, truncate);
        auto formatterDefault = std::make_unique<spdlog::pattern_formatter>();
        formatterDefault->add_flag<CustomMsgCallbackFlag>(CustomMsgCallbackFlag::flag, _this.prefixCallback).set_pattern(GetPattern(Pattern::Default));
        _this.fileSink->set_formatter(std::move(formatterDefault));
        _this.fileSink->set_level(spdlog::level::trace);

        _this.fileSinkRaw = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, truncate);
        _this.fileSinkRaw->set_pattern(GetPattern(Pattern::Raw));
        _this.fileSinkRaw->set_level(spdlog::level::trace);

        _this.fileSinkTime = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, truncate);
        _this.fileSinkTime->set_pattern(GetPattern(Pattern::Time));
        _this.fileSinkTime->set_level(spdlog::level::trace);

        _this.fileSinkFunc = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, truncate);
        auto formatterFunc = std::make_unique<spdlog::pattern_formatter>();
        formatterFunc->add_flag<CustomMsgCallbackFlag>(CustomMsgCallbackFlag::flag, _this.prefixCallback).set_pattern(GetPattern(Pattern::Func));
        _this.fileSinkFunc->set_formatter(std::move(formatterFunc));
        _this.fileSinkFunc->set_level(spdlog::level::trace);

        _this.fileSinkExtend = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, truncate);
        auto formatterExtend = std::make_unique<spdlog::pattern_formatter>();
        formatterExtend->add_flag<CustomMsgCallbackFlag>(CustomMsgCallbackFlag::flag, _this.prefixCallback, _this.postfixCallback).set_pattern(GetPattern(Pattern::Default));
        _this.fileSinkExtend->set_formatter(std::move(formatterExtend));
        _this.fileSinkExtend->set_level(spdlog::level::trace);


        _this.logger = std::make_shared<spdlog::logger>("logger", spdlog::sinks_init_list{ _this.fileSink });
        _this.logger->flush_on(spdlog::level::trace);
        _this.logger->set_level(spdlog::level::trace);

        _this.rawLogger = std::make_shared<spdlog::logger>("raw_logger", spdlog::sinks_init_list{ _this.fileSinkRaw });
        _this.rawLogger->flush_on(spdlog::level::trace);
        _this.rawLogger->set_level(spdlog::level::trace);

        _this.timeLogger = std::make_shared<spdlog::logger>("time_logger", spdlog::sinks_init_list{ _this.fileSinkTime });
        _this.timeLogger->flush_on(spdlog::level::trace);
        _this.timeLogger->set_level(spdlog::level::trace);

        _this.funcLogger = std::make_shared<spdlog::logger>("func_logger", spdlog::sinks_init_list{ _this.fileSinkFunc });
        _this.funcLogger->flush_on(spdlog::level::trace);
        _this.funcLogger->set_level(spdlog::level::trace);

#ifdef _DEBUG
        _this.debugLogger = std::make_shared<spdlog::logger>("debug_logger", spdlog::sinks_init_list{ _this.debugSink, _this.fileSink });
        _this.debugLogger->flush_on(spdlog::level::trace);
        _this.debugLogger->set_level(spdlog::level::trace);
#endif

#ifdef _DEBUG
        _this.extendLogger = std::make_shared<spdlog::logger>("extend_logger", spdlog::sinks_init_list{ _this.debugSink, _this.fileSinkExtend });
#else
        _this.extendLogger = std::make_shared<spdlog::logger>("extend_logger", spdlog::sinks_init_list{ _this.fileSinkExtend });
#endif
        _this.extendLogger->flush_on(spdlog::level::trace);
        _this.extendLogger->set_level(spdlog::level::trace);


        if (appendNewSessionMsg) {
            _this.rawLogger->info("\n");
            // whitespaces are selected by design
            _this.rawLogger->info("==========================================================================================================");
            _this.timeLogger->info("                       New session started");
            _this.rawLogger->info("==========================================================================================================");
        }
    }

    std::string DefaultLoggers::GetLastMessage() {
        std::unique_lock lk{ GetInstance().mxCustomFlagHandlers };
        return GetInstance().lastMessage;
    }

    std::shared_ptr<spdlog::logger> DefaultLoggers::Logger() {
        auto& _this = GetInstance(); // ensure that token set

        if (TokenSingleton<DefaultLoggers>::IsExpired() || !_this.logger) {
            return TokenSingleton<DefaultLoggers>::GetData().DefaultLogger();
        }
        return GetInstance().logger;
    }

    std::shared_ptr<spdlog::logger> DefaultLoggers::RawLogger() {
        auto& _this = GetInstance();

        if (TokenSingleton<DefaultLoggers>::IsExpired() || !_this.rawLogger) {
            return TokenSingleton<DefaultLoggers>::GetData().DefaultLogger();
        }
        return GetInstance().rawLogger;
    }

    std::shared_ptr<spdlog::logger> DefaultLoggers::TimeLogger() {
        auto& _this = GetInstance();

        if (TokenSingleton<DefaultLoggers>::IsExpired() || !_this.timeLogger) {
            return TokenSingleton<DefaultLoggers>::GetData().DefaultLogger();
        }
        return GetInstance().timeLogger;
    }

    std::shared_ptr<spdlog::logger> DefaultLoggers::FuncLogger() {
        auto& _this = GetInstance();

        if (TokenSingleton<DefaultLoggers>::IsExpired() || !_this.funcLogger) {
            return TokenSingleton<DefaultLoggers>::GetData().DefaultLogger();
        }
        return GetInstance().funcLogger;
    }

    std::shared_ptr<spdlog::logger> DefaultLoggers::DebugLogger() {
#ifdef _DEBUG
        auto& _this = GetInstance();

        if (TokenSingleton<DefaultLoggers>::IsExpired() || !_this.debugLogger) {
            return TokenSingleton<DefaultLoggers>::GetData().DefaultLogger();
        }
        return GetInstance().debugLogger;
#else
        return Logger();
#endif
    }

    std::shared_ptr<spdlog::logger> DefaultLoggers::ExtendLogger() {
        auto& _this = GetInstance();

        if (TokenSingleton<DefaultLoggers>::IsExpired() || !_this.extendLogger) {
            return TokenSingleton<DefaultLoggers>::GetData().DefaultLogger();
        }
        return GetInstance().extendLogger;
    }
}

H::nothing* __LgCtx() {
    return nullptr;
}