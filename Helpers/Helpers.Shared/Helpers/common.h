#pragma once
#ifndef HELPERS_NS_ALIAS
#define HELPERS_NS_ALIAS H
#endif

#define HELPERS_NS __helpers_namespace
namespace HELPERS_NS {} // create uniq "helpers namespace" for this project
namespace HELPERS_NS_ALIAS = HELPERS_NS; // set your alias for original "helpers namespace" (defined via macro)

#include <winapifamily.h>

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define COMPILE_FOR_DESKTOP 1
#endif

#if (_MANAGED == 1) || (_M_CEE == 1)
// COMPILE_FOR_DESKTOP also == 1
#define COMPILE_FOR_CLR 1
#else
#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define COMPILE_FOR_WINRT 1
#endif
#endif


/* ------------------------------------ */
/*   Check external includes / nugets   */
/* ------------------------------------ */
#if !defined(__has_include)
#pragma message("'__has_include' directive not found")
#endif

#if defined(CRASH_HANDLING_NUGET) || __has_include("CrashHandling/CrashHandling.h")
#define CRASH_HANDLING_SUPPORT
#endif

#if __has_include("boost/asio.hpp")
#define BOOST_SUPPORTED
#endif