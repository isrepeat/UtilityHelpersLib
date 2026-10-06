#pragma once
#include "../Bridge/PreviewPluginSdkTypes.h"

#include <cstdint>

namespace preview_sdk::abi {
    //
    // Фасад сведений о плагине и жизненного цикла C ABI session handle.
    //
    class PreviewPluginAbi final {
    public:
        static const char* LastError();
        static uint32_t AbiVersion();
        static bool WritePluginInfo(char* destination, int capacity);
        static AndroidAppPreviewerPluginSDK::xp_session* CreateSession(int width, int height);
        static void DestroySession(AndroidAppPreviewerPluginSDK::xp_session* session);
    };
}