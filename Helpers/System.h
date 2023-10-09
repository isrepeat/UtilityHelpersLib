#pragma once
#include "Helpers.h"
#include "Macros.h"
#include "HWindows.h"
#include <stdexcept>
#include <comdef.h>
#include <memory>


namespace H {
    namespace System {
#ifdef CRASH_HANDLING_NUGET
        // NOTE: For unqie_ptr over pipmpl you need define Dtor (even default) in .cpp file
        class Backtrace;
#endif

        class ComException : public std::exception {
        public:
            ComException(HRESULT hr, const std::wstring& message);
            ~ComException() = default;

            ComException(const ComException&) = default;
            ComException& operator=(const ComException&) = default;

            NO_MOVE(ComException); // std::exception not support move Ctor

#ifdef CRASH_HANDLING_NUGET
            const std::shared_ptr<Backtrace>& GetBacktrace() const;
            void LogBacktrace() const;
#endif

            std::wstring ErrorMessage() const;
            HRESULT ErrorCode() const;

        private:
#ifdef CRASH_HANDLING_NUGET
            std::shared_ptr<Backtrace> backtrace;
#endif
            std::wstring errorMessage;
            HRESULT errorCode = S_OK;
        };

        inline void ThrowIfFailed(HRESULT hr) {
            if (FAILED(hr)) {
                throw ComException(hr, _com_error(hr).ErrorMessage());
            }
        }
    }
}