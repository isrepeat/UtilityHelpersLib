#include "LoggerNative.h"
#include "Helpers\Text.h"

extern "C" {
    CPPFEATURES_API void CppFeatures_LoggerInit(const wchar_t* logFilePath, int initFlags) {
        try {
            CppFeatures::Logger::Init(
                logFilePath == nullptr ? L"" : logFilePath,
                static_cast<CppFeatures::Logger::InitFlags>(initFlags)
            );
        }
        catch (...) {
            // Исключения не должны пересекать C ABI и границу P/Invoke.
        }
    }

    CPPFEATURES_API void CppFeatures_LoggerLog(
        const wchar_t* message,
        const wchar_t* filePath,
        const wchar_t* memberName,
        int lineNumber,
        int level
    ) {
        try {
            CppFeatures::Logger::Context context{
                filePath == nullptr ? "" : HELPERS_NS::Text::Utf16ToUtf8(filePath),
                memberName == nullptr ? "" : HELPERS_NS::Text::Utf16ToUtf8(memberName),
                lineNumber
            };

            CppFeatures::Logger::Log(
                context,
                CppFeatures::Logger::Pattern::Default,
                static_cast<CppFeatures::Logger::Level>(level),
                message == nullptr ? L"" : message
            );
        }
        catch (...) {
            // Исключения не должны пересекать C ABI и границу P/Invoke.
        }
    }
}
