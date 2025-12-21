#pragma once
#include "common.h"
#include "Exceptions.h"
#include "Logger.h"
#include <comdef.h>
#include <exception>
#if defined(__cppwinrt) || defined(WINRT_BASE_H)
#include <winrt/base.h>
#endif

namespace HELPERS_NS {
    namespace System {
#if COMPILE_FOR_DESKTOP
        inline void ThrowIfFailed(HRESULT hr) {
            if (FAILED(hr)) {
                LOG_FAILED(hr);
                Dbreak;
                throw ComException(hr, _com_error(hr).ErrorMessage());
            }
        }

        inline ComException MakeWin32Exception() {
            auto err = GetLastError();
            HRESULT hr = E_FAIL;

            if (err != ERROR_SUCCESS) {
                hr = HRESULT_FROM_WIN32(err);
                LOG_FAILED(hr);
            }

            return ComException(hr, _com_error(hr).ErrorMessage());
        }
#endif
#if COMPILE_FOR_CX_or_WINRT
        inline void ThrowIfFailed(HRESULT hr) {
            if (FAILED(hr)) {
                LOG_FAILED(hr);
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
                    LOG_FAILED(hr);
                    Dbreak;
                    LOG_ERROR_D(L"Com exception = [{:#08x}] [WinRt]", static_cast<unsigned int>(hr));
                    throw ref new Platform::COMException(hr);
                }
            }
        }
#endif

        inline void LogCurrentException(const wchar_t* where) {
            LOG_DEBUG_D(L"[EXCEPTION] {}:", where);

            std::exception_ptr exceptionPtr = std::current_exception();
            if (!exceptionPtr) {
                LOG_DEBUG_D(L"[EXCEPTION] {}: no active exception.", where);
                return;
            }

            try {
                std::rethrow_exception(exceptionPtr);
            }
            catch (const _com_error& comError) {
                const HRESULT hr = comError.Error();
                const wchar_t* msgPtr = comError.ErrorMessage();

                LOG_ERROR_D(
                    L"_com_error:\n"
                    L"hr  = 0x{:08x}\n"
                    L"msg = {}\n"
                    , static_cast<unsigned int>(hr)
                    , (msgPtr != nullptr) ? msgPtr : L"(null)"
                );
            }
#if defined(__cppwinrt) || defined(WINRT_BASE_H)
            catch (const winrt::hresult_error& winrtError) {
                const HRESULT hr = static_cast<HRESULT>(winrtError.code());
                const wchar_t* msgPtr = winrtError.message().c_str();

                LOG_ERROR_D(
                    L"winrt::hresult_error:\n"
                    L"hr  = 0x{:08x}\n"
                    L"msg = {}\n",
                    static_cast<unsigned int>(hr),
                    (msgPtr != nullptr) ? msgPtr : L"(null)"
                );
            }
#endif
#ifdef __cplusplus_winrt
            catch (Platform::Exception^ cxException) {
                const HRESULT hr = cxException->HResult;
                const wchar_t* msgPtr = cxException->Message != nullptr ? cxException->Message->Data() : L"(null)";

                LOG_ERROR_D(
                    L"Platform::Exception:\n"
                    L"hr  = 0x{:08x}\n"
                    L"msg = {}\n",
                    static_cast<unsigned int>(hr),
                    msgPtr
                );
            }
#endif
            catch (const std::exception& stdException) {
                LOG_ERROR_D("std::exception:\n"
                    "what = {}\n",
                    stdException.what()
                );
            }
            catch (...) {
                LOG_ERROR_D("unknown non-std exception");
            }
        }
    }
}