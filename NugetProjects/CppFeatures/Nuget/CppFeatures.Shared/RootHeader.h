#pragma once
#define CPPFEATURES_API // by default is empty (also used as static library)

#ifdef __MAKE_DYNAMIC_LIBRARY__
// [Case when this file is included from internal projects] (NUGET_HELPERS_NS defined at Shared.targets)
// NOTE: Define all namespace macros in predefined section, because this project use .Shared
//       sources of another projects which can be compiled before this header is evaluated.
#define CPPFEATURES_API __declspec(dllexport)
#else
#ifdef CPPFEATURES_NUGET
// [Case when this file is included from client project] (NUGET_HELPERS_NS defined at nuget .targets)
#define CPPFEATURES_API __declspec(dllimport)
#else
// [Case when this file is included from Cx.WRC project]
#define NUGET_HELPERS_NS CppFeaturesHelpers
#endif
#endif

// Use "" to include Helpers files from nuget includes dir
#include "Helpers\common.h"
#include <functional>
#include <vector>
#include <string>
#include <memory>