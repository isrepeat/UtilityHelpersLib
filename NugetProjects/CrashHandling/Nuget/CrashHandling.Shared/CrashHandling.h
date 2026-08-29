#pragma once
#include "RootHeader.h"
#include <functional>
#include <utility>
#include <vector>
#include <string>

namespace CrashHandling {
    enum class ExceptionType {
        StructuredException,
        UnhandledException,
    };

    struct BacktraceFrame {
        std::wstring moduleName;
        std::uint32_t RVA = 0;
    };

    struct StackFrame {
        std::wstring symbolName;
        std::wstring filename;
        std::uint32_t lineNumber = 0;
        std::uint32_t RVA = 0;
    };

    using Backtrace_t = std::vector<std::pair<std::wstring, std::vector<BacktraceFrame>>>; // use vector pair to keep insertion order

    struct AdditionalInfo {
        std::wstring appCenterId;
        std::wstring appVersion; // if app version passed empty it is used current package app version or 1.0.0.0 (if it is desktop)
        std::wstring appUuid = L"00000000-0000-0000-0000-000000000001"; // unique client app id
        std::wstring backtrace;
        std::wstring exceptionMsg;
        std::wstring channelName = L"\\\\.\\pipe\\Local\\channelDumpWriter";
    };

    CRASH_HANDLING_API Backtrace_t GetBacktrace(int SkipFrames);
    CRASH_HANDLING_API std::wstring BacktraceToString(const Backtrace_t& backtrace);

    class CRASH_HANDLING_API Backtrace {
    public:
        Backtrace(int skipFrames = 0);
        Backtrace_t GetBacktrace() const;
        std::wstring GetBacktraceStr() const;

    private:
        Backtrace_t backtrace;
    };

    // CHECK: Need compile this project with /EHa (need for Release)
    CRASH_HANDLING_API void RegisterVectorHandler(PVECTORED_EXCEPTION_HANDLER handler);

    // Should call RegisterDefaultCrashHandler from main (UI) thread.
    // WARNING: Do not use async calls inside callback, after callback finished program call exit(0) after 'waitBeforeExitMs'.
    CRASH_HANDLING_API void RegisterDefaultCrashHandler(std::function<void(EXCEPTION_POINTERS*, ExceptionType)> crashCallback, int waitBeforeExitMs = 2'000);

    // Should be called inside callback that RegisterDefaultCrashHandler set. Send report timeout = 5s.
    CRASH_HANDLING_API void GenerateCrashReport(
        EXCEPTION_POINTERS* pExceptionPtrs,
        const AdditionalInfo& additionalInfo,
        const std::wstring& runProtocolMinidumpWriter,
        const std::vector<std::pair<std::wstring, std::wstring>>& commandArgs = {},
        std::function<void(const std::wstring&)> callbackToRunProtocol = nullptr);

    class CRASH_HANDLING_API CrashHandler {
        CrashHandler() = delete;
        ~CrashHandler() = delete;

    public:
        // Should init CrashHandler from main (UI) thread
        static void Init(std::wstring runProtocol, std::wstring appCenterId, std::wstring appUuid);
        static void SetCrashCallback(CRASH_HANDLING_NUGET_HELPERS_NS::Callback<void> crashCallback);
        static void SetFinishCallback(CRASH_HANDLING_NUGET_HELPERS_NS::Callback<void> finishCallback);
        static void SetProtocolCommandArgs(std::vector<std::pair<std::wstring, std::wstring>> protocolCommandArgs);
        static void SetRunProtocolWithParamsCallback(CRASH_HANDLING_NUGET_HELPERS_NS::Callback<void, const std::wstring&> runProtocolWithParamsCallback);
    };
}