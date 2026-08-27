#include "WinApi.h"
#include "../CppFeatures.Shared/WinApiNative.h"

namespace CppFeatures::Cx::WinApi {
    HWND GetDesktopWindow() {
        return CppFeatures::Desktop::WinApi::GetDesktopWindow();
    }

    bool GetWindowRect(HWND window, LPRECT rect) {
        return CppFeatures::Desktop::WinApi::GetWindowRect(window, rect);
    }
}
