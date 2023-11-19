#pragma once
#include <spdlog/LogHelpers.h>
#include "Helpers.h"
#include "Thread.h"
#include "Macros.h"
#include "Scope.h"

#if !defined(DISABLE_ERROR_LOGGING)
#define LogLastError LOG_ERROR_D(L"Last error: {}", H::GetLastErrorAsString())

#define LOG_ASSERT(expression, message, ...)                                                                                  \
	if (!(expression)) {                                                                                                      \
		assertm(expression, message);                                                                                         \
		LOG_ERROR_D(message, __VA_ARGS__);                                                                                    \
	}

// NOTE: use comma instead semicolon
#define LOG_THROW_STD_EXCEPTION(fmt, ...)                                                                                     \
		LOG_ERROR_EX(fmt, __VA_ARGS__),  /* LOG_ERROR_EX saved last logger message */                                         \
		LogLastError,                                                                                                         \
		throw std::exception(lg::DefaultLoggers::GetLastMessage().c_str())                                                   

#else
#define LogLastError
#define LOG_ASSERT(expression, message, ...)
#define LOG_STD_EXCEPTION(...)
#endif


#if !defined(DISABLE_VERBOSE_LOGGING)
#define LOG_DEBUG_VERBOSE(fmt, ...) LOG_DEBUG_D(fmt, __VA_ARGS__)
#define LOG_ERROR_VERBOSE(fmt, ...) LOG_ERROR_D(fmt, __VA_ARGS__)

#define LOG_DEBUG_VERBOSE_S(_This, fmt, ...) LOG_DEBUG_S(_This, fmt, __VA_ARGS__)
#define LOG_ERROR_VERBOSE_S(_This, fmt, ...) LOG_ERROR_S(_This, fmt, __VA_ARGS__)
#else
#define LOG_DEBUG_VERBOSE(fmt, ...)
#define LOG_ERROR_VERBOSE(fmt, ...)

#define LOG_DEBUG_VERBOSE_S(_This, fmt, ...)
#define LOG_ERROR_VERBOSE_S(_This, fmt, ...)
#endif


#define LOG_THREAD(name)                                                                                                     \
	LOG_DEBUG_D(L"Thread START '{}'", std::wstring(name));                                                                   \
	H::ThreadNameHelper::SetThreadName(name);                                                                                \
                                                                                                                             \
	auto threadFinishLogScoped = H::MakeScope([&] {                                                                          \
		LOG_DEBUG_D(L"Thread END '{}'", std::wstring(name));                                                                 \
		});
