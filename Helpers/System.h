#pragma once
#include "Helpers.h"
#include "Macros.h"
#include "HWindows.h"
#include <stdexcept>
#include <comdef.h>
#include <memory>


namespace H {
    namespace System {
        // NOTE: For unqie_ptr over pipmpl you need define Dtor (even default) in .cpp file
        class Backtrace;

        class ComException : public std::exception {
        public:
            ComException(HRESULT hr, const std::wstring& message);
            ~ComException() = default;

            ComException(const ComException&) = default;
            ComException& operator=(const ComException&) = default;

            NO_MOVE(ComException); // std::exception not support move Ctor

            const std::shared_ptr<Backtrace>& GetBacktrace() const;
            std::wstring ErrorMessage() const;
            HRESULT ErrorCode() const;

            std::wstring GetStacktrace() const;
            void LogStacktrace() const;

        private:
            std::shared_ptr<Backtrace> backtrace;
            mutable std::wstring stacktrace;
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