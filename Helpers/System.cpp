#include "System.h"
#include "Logger.h"
#include "Backtrace.h"
#include "StackTrace.h"


namespace H {
    namespace System {
        ComException::ComException(HRESULT hr, const std::wstring& message)
            : std::exception(H::WStrToStr(message).c_str())
            , backtrace{ std::make_shared<Backtrace>() }
            , errorMessage{message}
            , errorCode{hr}
        {
            LOG_ERROR_D("Com exception = [{:#08x}] {}", static_cast<unsigned int>(hr), H::WStrToStr(message));
        }

        const std::shared_ptr<Backtrace>& ComException::GetBacktrace() const {
            return backtrace;
        }

        std::wstring ComException::ErrorMessage() const {
            return errorMessage;
        }

        HRESULT ComException::ErrorCode() const {
            return errorCode;
        }


        std::wstring ComException::GetStacktrace() const {
            if (stacktrace.size()) {
                return stacktrace;
            }

            return stacktrace = H::System::BuildStacktrace(backtrace);
        }

        void ComException::LogStacktrace() const {
            LOG_ERROR_D("Com exception = [{:#08x}] {}", static_cast<unsigned int>(errorCode), H::WStrToStr(errorMessage));
            LOG_ERROR_D(L"\n\nStacktrace: \n{}", this->GetStacktrace());
        }
    }
}