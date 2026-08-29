#pragma once
#include "RootHeader.h"

namespace CppFeatures::Desktop::WinApi {
    CPPFEATURES_API HWND GetDesktopWindow();
    CPPFEATURES_API bool GetWindowRect(HWND window, LPRECT rect);
}
