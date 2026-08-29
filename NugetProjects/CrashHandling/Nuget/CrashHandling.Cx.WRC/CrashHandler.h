#pragma once
#include "NamespacesAliases.h"
#include <string>

namespace CrashHandling {
    namespace Cx {
        public delegate void CrashEventCallback();

        public ref class CrashHandlerWinRT sealed {
        private:
            CrashHandlerWinRT(Platform::String^ minidumpWriterProtocolName, Platform::String^ appCenterId, Platform::String^ appUuid);
            static CrashHandlerWinRT^ instance;

        public:
            static CrashHandlerWinRT^ CreateInstance(Platform::String^ minidumpWriterProtocolName, Platform::String^ appCenterId, Platform::String^ appUuid);
            static CrashHandlerWinRT^ GetInstance();

            virtual ~CrashHandlerWinRT() {}

            event CrashEventCallback^ CrashEvent;

        private:
            static void CrashCallback(CrashHandlerWinRT^ _this);
            static void RunProtocolWithParamsCallback(CrashHandlerWinRT^ _this, const std::wstring& protocolWithParams);

        private:
            const std::size_t initializedThreadId;
        };
    }
}