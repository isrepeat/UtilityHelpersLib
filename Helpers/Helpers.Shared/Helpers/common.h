#pragma once
#include "Preprocessor.h"

#ifndef HELPERS_NS_ALIAS
#define HELPERS_NS_ALIAS H
#else
#pragma message(PREPROCESSOR_MSG("HELPERS_NS_ALIAS already defined = '" PP_STRINGIFY(HELPERS_NS_ALIAS) "'"))
#endif

#define HELPERS_NS __H_ns
namespace HELPERS_NS {} // create uniq "helpers namespace" for this project
namespace HELPERS_NS_ALIAS = HELPERS_NS; // set your alias for original "helpers namespace" (defined via macro)


#ifndef MEDIA_FOUNDATION_NS_ALIAS
#define MEDIA_FOUNDATION_NS_ALIAS MF
#else
#pragma message(PREPROCESSOR_MSG("MEDIA_FOUNDATION_NS_ALIAS already defined = '" PP_STRINGIFY(MEDIA_FOUNDATION_NS_ALIAS) "'"))
#endif

#define MEDIA_FOUNDATION_NS __MF_ns
namespace MEDIA_FOUNDATION_NS {}
namespace MEDIA_FOUNDATION_NS_ALIAS = MEDIA_FOUNDATION_NS;


#include <winapifamily.h>

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define COMPILE_FOR_DESKTOP 1
#endif

#if (_MANAGED == 1) || (_M_CEE == 1)
// COMPILE_FOR_DESKTOP also == 1
#define COMPILE_FOR_CLR 1
#else
#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define COMPILE_FOR_CX_or_WINRT 1

#ifdef __cplusplus_winrt
#	define COMPILE_FOR_WINRT 1
#else
#	define COMPILE_FOR_CX 1
#endif
#endif
#endif


#if COMPILE_FOR_WINRT
#include <collection.h>
#include <ppltasks.h>

namespace PCollections = Platform::Collections;

namespace WFCollections = Windows::Foundation::Collections;
namespace WFoundation = Windows::Foundation;
namespace WSStreams = Windows::Storage::Streams;
namespace WStorage = Windows::Storage;
#endif


/* ------------------------------------ */
/*   Check external includes / nugets   */
/* ------------------------------------ */
#if !defined(__has_include)
#pragma message("'__has_include' directive not found")
#endif

#if defined(CRASH_HANDLING_NUGET) || __has_include("CrashHandling/CrashHandling.h")
#define CRASH_HANDLING_SUPPORT 1
#endif

#if defined(SPDLOG_SOURCES) || __has_include("Spdlog/LogHelpers.h")
#define SPDLOG_SUPPORT 1
#endif

#if __has_include("boost/asio.hpp")
#define BOOST_SUPPORTED 1
#endif