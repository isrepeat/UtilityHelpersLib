#pragma once
#include <windows.h>

#ifdef CPPFEATURES_CX_WRC_EXPORTS
#define CPPFEATURES_CX_API __declspec(dllexport)
#else
#define CPPFEATURES_CX_API __declspec(dllimport)
#endif

namespace CppFeatures::Cx::WinApi {
    CPPFEATURES_CX_API HWND GetDesktopWindow();
    CPPFEATURES_CX_API bool GetWindowRect(HWND window, LPRECT rect);
}
