#pragma once
#include "common.h"
#include "HWindows.h"
#include "Exception.h"
#include "Helpers.h"
#include "Logger.h" // TODO: because of System.h implicitly included in Logger.h the Logger.h macros not defined, fix in future
#include "Macros.h"

#include <stdexcept>
#include <comdef.h>
#include <memory>

namespace CrashHandling {
    class Backtrace;
}

namespace HELPERS_NS {
    namespace System {
        // NOTE: For unqie_ptr over pipmpl you need define Dtor (even default) in .cpp file
#if COMPILE_FOR_DESKTOP
        class ComException : public std::exception {
        public:
            ComException(HRESULT hr, const std::wstring& message);
            ~ComException() = default;

            ComException(const ComException&) = default;
            ComException& operator=(const ComException&) = default;

            NO_MOVE(ComException); // std::exception not support move Ctor

            // [may throw NugetNotFoundException]
            const std::shared_ptr<CrashHandling::Backtrace>& GetBacktrace() const;
            void LogBacktrace() const;

            std::wstring ErrorMessage() const;
            HRESULT ErrorCode() const;

        private:
            std::shared_ptr<CrashHandling::Backtrace> backtrace;
            std::wstring errorMessage;
            HRESULT errorCode = S_OK;
        };

        inline void ThrowIfFailed(HRESULT hr) {
            static_assert(std::is_same_v<TCHAR, wchar_t>, "TCHAR != wchar_t");
            if (FAILED(hr)) {
                Dbreak;
                throw ComException(hr, _com_error(hr).ErrorMessage());
            }
        }
#endif
#if COMPILE_FOR_CX_or_WINRT
        inline void ThrowIfFailed(HRESULT hr) {
            if (FAILED(hr)) {
                Dbreak;
                auto comErr = _com_error(hr, nullptr);
                LOG_ERROR_D(L"Com exception = [{:#08x}] [Cx / WinRt]", static_cast<unsigned int>(hr));
                throw comErr;
            }
        }
#endif
#if COMPILE_FOR_WINRT
        namespace WinRt {
            inline void ThrowIfFailed(HRESULT hr) {
                if (FAILED(hr)) {
                    Dbreak;
                    LOG_ERROR_D(L"Com exception = [{:#08x}] [WinRt]", static_cast<unsigned int>(hr));
                    throw ref new Platform::COMException(hr);
                }
            }
        }
#endif
    }
}