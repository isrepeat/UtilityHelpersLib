#pragma once
#include <spdlog/LogHelpers.h>
#include "Helpers.h"
#include "Thread.h"
#include "Macros.h"
#include "Scope.h"

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

#define LOG_DEBUG_VERBOSE_S(_This, ...) LOG_DEBUG_S(_This, __VA_ARGS__)
#define LOG_ERROR_VERBOSE_S(_This, ...) LOG_ERROR_S(_This, __VA_ARGS__)
#else
#define LOG_DEBUG_VERBOSE(...)
#define LOG_ERROR_VERBOSE(...)

#define LOG_DEBUG_VERBOSE_S(_This, ...)
#define LOG_ERROR_VERBOSE_S(_This, ...)
#endif


#define LOG_THREAD(name)                                                                                                     \
	LOG_DEBUG_D(L"Thread START '{}'", std::wstring(name));                                                                   \
	H::ThreadNameHelper::SetThreadName(name);                                                                                \
                                                                                                                             \
	auto threadFinishLogScoped = H::MakeScope([&] {                                                                          \
		LOG_DEBUG_D(L"Thread END '{}'", std::wstring(name));                                                                 \
		});
