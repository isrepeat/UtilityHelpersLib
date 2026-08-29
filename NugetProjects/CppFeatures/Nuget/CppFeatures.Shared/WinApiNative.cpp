#include "WinApiNative.h"

namespace CppFeatures::Desktop::WinApi {
    HWND GetDesktopWindow() {
        return ::GetDesktopWindow();
    }

    bool GetWindowRect(HWND window, LPRECT rect) {
        return ::GetWindowRect(window, rect) != FALSE;
    }
}
