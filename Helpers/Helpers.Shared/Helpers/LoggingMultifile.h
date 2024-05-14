#pragma once
#include "common.h"
#if SPDLOG_SUPPORT
#include <Spdlog/LogHelpers.h>
#else
#define DISABLE_MULTIFILE_LOGGING
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