#include "System.h"
#include "Logger.h"
#ifndef __HELPERS_RAW__
#include "Backtrace.h"
#endif


namespace H {
    namespace System {
        ComException::ComException(HRESULT hr, const std::wstring& message)
            : std::exception(H::WStrToStr(message).c_str())
#ifndef __HELPERS_RAW__
            , backtrace{ std::make_shared<Backtrace>() }
#endif
            , errorMessage{message}
            , errorCode{hr}
        {
            LOG_ERROR_D(L"Com exception = [{:#08x}] {}", static_cast<unsigned int>(hr), message);
        }

#ifndef __HELPERS_RAW__
        const std::shared_ptr<Backtrace>& ComException::GetBacktrace() const {
            return backtrace;
        }

        void ComException::LogBacktrace() const {
            LOG_ERROR_D(L"Com exception = [{:#08x}] {}", static_cast<unsigned int>(errorCode), errorMessage);
            LOG_ERROR_D(L"\n\Backtrace: \n{}", backtrace->GetBacktraceStr());
        }
#endif

        std::wstring ComException::ErrorMessage() const {
            return errorMessage;
        }

        HRESULT ComException::ErrorCode() const {
            return errorCode;
        }
    }
}