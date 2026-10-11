#include "PreviewPluginAbi.h"

#include "../../Session/PluginDefinition.h"
#include "../Bridge/Diagnostic.h"
#include "../Bridge/TextBuffer.h"

#include <stdexcept>

namespace preview_sdk::abi {
    const char* PreviewPluginAbi::LastError() {
        return bridge::LastError().c_str();
    }

    uint32_t PreviewPluginAbi::AbiVersion() {
        return AndroidAppPreviewerPluginSDK::android_app_previewer_plugin_abi_version;
    }

    bool PreviewPluginAbi::WritePluginInfo(char* destination, int capacity) {
        if (destination == nullptr || capacity <= 0) {
            throw std::invalid_argument("Plugin-info buffer and positive capacity are required");
        }
        bridge::TextBuffer::Write(
            PluginInfoJson(),
            destination,
            static_cast<size_t>(capacity),
            "Plugin-info buffer is too small"
        );
        return true;
    }

    AndroidAppPreviewerPluginSDK::xp_session* PreviewPluginAbi::CreateSession(int width, int height) {
        return new AndroidAppPreviewerPluginSDK::xp_session(width, height);
    }

    void PreviewPluginAbi::DestroySession(AndroidAppPreviewerPluginSDK::xp_session* session) {
        delete session;
    }
}