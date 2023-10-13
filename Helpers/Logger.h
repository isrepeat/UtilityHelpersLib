#pragma once
#include <spdlog/LogHelpers.h>
#include "Helpers.h"
#include "Macros.h"

#if !defined(DISABLE_ERROR_LOGGING)
#define LogLastError LOG_ERROR_D(L"Last error: {}", H::GetLastErrorAsString())
#define LOG_ASSERT(assertion, message, ...)                                                                                   \
	if (!assertion) {                                                                                                         \
		assert(assertion && " --> " message);                                                                                 \
		LOG_ERROR_D(EXPAND_1_VA_ARGS_(message, __VA_ARGS__));                                                                 \
	}
#else
#define LogLastError
#define LOG_ASSERT(assertion, message, ...)
#endif


#if !defined(DISABLE_VERBOSE_LOGGING)
#define LOG_DEBUG_VERBOSE(...) LOG_DEBUG_D(__VA_ARGS__)
#define LOG_ERROR_VERBOSE(...) LOG_ERROR_D(__VA_ARGS__)
#else
#define LOG_DEBUG_VERBOSE(...)
#define LOG_ERROR_VERBOSE(...)
#endif


#if !defined(DISABLE_CLASS_FULLNAME_LOGGING)
#define CLASS_FULLNAME_LOGGING_INLINE_IMPLEMENTATION(className)                                                               \
	private:                                                                                                                  \
		std::string className##_fullClassNameA = ""#className;                                                                \
		std::wstring className##_fullClassNameW = L""#className;                                                              \
                                                                                                                              \
	public:                                                                                                                   \
        void SetFullClassName(std::wstring name) {                                                                            \
            LOG_DEBUG_D(L"Full class name = {}", name);                                                                       \
            this->className##_fullClassNameA = H::WStrToStr(name);                                                            \
            this->className##_fullClassNameW = name;                                                                          \
        }                                                                                                                     \
                                                                                                                              \
		const std::string& GetFullClassNameA() {                                                                              \
			return this->className##_fullClassNameA;                                                                          \
		}                                                                                                                     \
                                                                                                                              \
		const std::wstring& GetFullClassNameW() {                                                                             \
			return this->className##_fullClassNameW;                                                                          \
		}


#define LogFuncWithFullClassNameA(format, ...)                                                                                \
	LOG_FUNCTION_ENTER(std::string("[{}] ") + format, EXPAND_1_VA_ARGS_(this->GetFullClassNameA(), __VA_ARGS__))

#define LogDebugWithFullClassNameA(format, ...)                                                                                \
	LOG_DEBUG_D(std::string("[{}] ") + format, EXPAND_1_VA_ARGS_(this->GetFullClassNameA(), __VA_ARGS__))

#define LogErrorWithFullClassNameA(format, ...)                                                                                \
	LOG_ERROR_D(std::string("[{}] ") + format, EXPAND_1_VA_ARGS_(this->GetFullClassNameA() , __VA_ARGS__))

#define LogWarningWithFullClassNameA(format, ...)                                                                              \
	LOG_WARNING_D(std::string("[{}] ") + format, EXPAND_1_VA_ARGS_(this->GetFullClassNameA() , __VA_ARGS__))


#define LogFuncWithFullClassNameW(format, ...)                                                                                \
	LOG_DEBUG_D(std::wstring(L"[{}] ") + format, EXPAND_1_VA_ARGS_(this->GetFullClassNameW() , __VA_ARGS__))

#define LogDebugWithFullClassNameW(format, ...)                                                                                \
	LOG_DEBUG_D(std::wstring(L"[{}] ") + format, EXPAND_1_VA_ARGS_(this->GetFullClassNameW() , __VA_ARGS__))

#define LogErrorWithFullClassNameW(format, ...)                                                                                \
	LOG_ERROR_D(std::wstring(L"[{}] ") + format, EXPAND_1_VA_ARGS_(this->GetFullClassNameW() , __VA_ARGS__))

#define LogWarningWithFullClassNameW(format, ...)                                                                              \
	LOG_WARNING_D(std::wstring(L"[{}] ") + format, EXPAND_1_VA_ARGS_(this->GetFullClassNameW() , __VA_ARGS__))

#else
#define CLASS_FULLNAME_LOGGING_INLINE_IMPLEMENTATION(className)
#define LogFuncWithFullClassNameA(formatStr, ...)
#define LogFuncWithFullClassNameW(formatStr, ...)
#define LogDebugWithFullClassNameA(formatStr, ...)
#define LogDebugWithFullClassNameW(formatStr, ...)
#define LogErrorWithFullClassNameA(formatStr, ...)
#define LogErrorWithFullClassNameW(formatStr, ...)
#define LogWarningWithFullClassNameA(formatStr, ...)
#define LogWarningWithFullClassNameW(formatStr, ...)
#endif