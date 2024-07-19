#include "HelpersWinApiCX.h"
#include <HelpersWinApi\Window.h>

namespace HELPERS_NS {
    namespace Cx {
        namespace WinApi {
            HELPERS_WINAPI_CX_API bool GetWindowRect(HWND hWnd, LPRECT lpRect) {
                return H::Desktop::GetWindowRect(hWnd, lpRect);
            }
        }
    }
}