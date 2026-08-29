#pragma once
#define CRASH_HANDLING_API // by default is empty (also used as static library)

#ifdef __MAKE_DYNAMIC_LIBRARY__
// [Internal]
// [Case when this file is included from internal projects]
// (XXX_NUGET_YYY_NS defined at Shared.targets)
// NOTE: Define all namespace macros in predefined section, because this project use .Shared
//       sources of another projects which can be compiled before this header is evaluated.
#	define CRASH_HANDLING_API __declspec(dllexport)
#else
// [Consumer]
#define CRASH_HANDLING_NUGET_HELPERS_NS __CrashHandlingH_ns
#define CRASH_HANDLING_NUGET_MEDIA_FOUNDATION_NS __CrashHandlingMF_ns

#ifdef CRASH_HANDLING_NUGET
// [Case when this file is included from client project]
#define CRASH_HANDLING_API __declspec(dllimport)
#else
// [Case when this file is included from Cx.WRC project]
#endif
#endif


#ifndef __MAKE_DYNAMIC_LIBRARY__
#pragma push_macro("HELPERS_NS")
#pragma push_macro("MEDIA_FOUNDATION_NS")
#pragma push_macro("HELPERS_NS_ALIAS")
#pragma push_macro("MEDIA_FOUNDATION_NS_ALIAS")
#define HELPERS_NS CRASH_HANDLING_NUGET_HELPERS_NS
#define MEDIA_FOUNDATION_NS CRASH_HANDLING_NUGET_MEDIA_FOUNDATION_NS

#define HELPERS_NS_ALIAS CrashHandlingHelpers
#define MEDIA_FOUNDATION_NS_ALIAS CrashHandlingMF
#endif


// NOTE:
// 1. Use "" to include Helpers files from huget includes dir
// 2. Ensure that all distributed nuget helpers is wrapped in #pragma push / pop macro construction.
#include "Helpers/common.h"
#include "Helpers/Callback.hpp"
#include "Helpers/TokenContext.hpp"

#ifndef __MAKE_DYNAMIC_LIBRARY__
#pragma pop_macro("MEDIA_FOUNDATION_NS_ALIAS")
#pragma pop_macro("HELPERS_NS_ALIAS")
#pragma pop_macro("MEDIA_FOUNDATION_NS")
#pragma pop_macro("HELPERS_NS")
#endif