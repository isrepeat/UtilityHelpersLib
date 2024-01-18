#pragma once
#include "common.h"
#include <Spdlog/LogHelpers.h>
#include "Helpers.h"
#include "Thread.h"
#include "System.h"
#include "Macros.h"
#include "Scope.h"

#if !defined(DISABLE_ERROR_LOGGING)
#define LogLastError LOG_ERROR_D(L"Last error: {}", HELPERS_NS::GetLastErrorAsString())
#define LogWSALastError LOG_ERROR_D(L"WSA Last error: {}", HELPERS_NS::GetWSALastErrorAsString())

// NOTE: 
// - use comma statement "|| (..., false)" to execute next expression
// - invert result to use it like "if (LOG_ASSERT(...))"
#define LOG_ASSERT(expression, message, ...)  !(																										\
	(bool)(expression) || (LOG_ERROR_D(L" " message L"  {" _CRT_WIDE(#expression) L"}", __VA_ARGS__), false) || (assertm(expression, message), false)	\
)

// NOTE: use comma instead semicolon to be able use it in "if statement" without braces
#define LOG_THROW_STD_EXCEPTION(fmt, ...)                                                                                     \
		LOG_ERROR_EX(fmt, __VA_ARGS__),  /* LOG_ERROR_EX saved last logger message */                                         \
		LogLastError,                                                                                                         \
		throw std::exception(lg::DefaultLoggers::GetLastMessage().c_str())                                                   

#define LOG_THROW_IF_FAILED(hr)                                                                                               \
	if (FAILED(hr)) {                                                                                                         \
		LogLastError;                                                                                                         \
		HELPERS_NS::System::ThrowIfFailed(hr);                                                                                \
	}

#else
#define LogLastError
#define LogWSALastError
#define LOG_ASSERT(expression, message, ...) !((bool)(expression))
#define LOG_THROW_STD_EXCEPTION(fmt, ...) throw std::exception(fmt)
#define LOG_THROW_IF_FAILED(hr, fmt, ...) HELPERS_NS::System::ThrowIfFailed(hr)
#endif


#if !defined(DISABLE_MULTIFILE_LOGGING)
#define LOG_RAW_N(loggerId, fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::RawLogger(loggerId), LOG_CTX, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_TIME_N(loggerId, fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::TimeLogger(loggerId), LOG_CTX, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))

#define LOG_DEBUG_N(loggerId, fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::Logger(loggerId), LOG_CTX, spdlog::level::debug, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_ERROR_N(loggerId, fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::Logger(loggerId), LOG_CTX, spdlog::level::err, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#define LOG_WARNING_N(loggerId, fmt, ...) lg::DefaultLoggers::Log<INNER_TYPE_STR(fmt)>(__LgCtx(), lg::DefaultLoggers::Logger(loggerId), LOG_CTX, spdlog::level::warn, EXPAND_1_VA_ARGS_(fmt, __VA_ARGS__))
#else
#define LOG_RAW_N(loggerId, fmt, ...)
#define LOG_TIME_N(loggerId, fmt, ...)

#define LOG_DEBUG_N(loggerId, fmt, ...)
#define LOG_ERROR_N(loggerId, fmt, ...)
#define LOG_WARNING_N(loggerId, fmt, ...)
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
	HELPERS_NS::ThreadNameHelper::SetThreadName(name);                                                                       \
                                                                                                                             \
	auto threadFinishLogScoped = HELPERS_NS::MakeScope([&] {                                                                 \
		LOG_DEBUG_D(L"Thread END '{}'", std::wstring(name));                                                                 \
		});
