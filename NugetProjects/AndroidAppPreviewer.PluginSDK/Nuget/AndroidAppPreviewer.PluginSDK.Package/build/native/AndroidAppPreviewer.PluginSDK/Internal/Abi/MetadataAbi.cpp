#include "MetadataAbi.h"

#include "../Bridge/PreviewPluginSdkTypes.h"
#include "../Bridge/Diagnostic.h"
#include "../Bridge/TextBuffer.h"
#include "PreviewPluginAbi.h"
#include "SessionAbi.h"

#include <string_view>
#include <stdexcept>
#include <string>
#include <vector>

namespace preview_sdk::abi {
    using namespace AndroidAppPreviewerPluginSDK;

    //
    // Методы
    //
    const char* MetadataAbi::xp_last_error(void) {
        return preview_sdk::abi::PreviewPluginAbi::LastError();
    }

    uint32_t MetadataAbi::xp_get_abi_version(void) {
        return preview_sdk::abi::PreviewPluginAbi::AbiVersion();
    }

    int MetadataAbi::xp_get_plugin_info(
        char* pluginInfoJson,
        int capacity
    ) {
        try {
            preview_sdk::bridge::LastError().clear();
            return preview_sdk::abi::PreviewPluginAbi::WritePluginInfo(pluginInfoJson, capacity) ? 1 : 0;
        } catch (const std::exception& error) {
            preview_sdk::bridge::LastError() = error.what();
            return 0;
        }
    }

    int MetadataAbi::xp_get_initial_page_id(
        void* session,
        char* pageId,
        int capacity
    ) {
        return preview_sdk::abi::SessionAbi::xp_session_current_page(static_cast<xp_session*>(session), pageId, capacity);
    }

    int MetadataAbi::xp_get_navigation_graph(
        void* session,
        char* graphJson,
        int capacity
    ) {
        try {
            preview_sdk::bridge::LastError().clear();
            if (session == nullptr || graphJson == nullptr || capacity <= 0) {
                throw std::invalid_argument("Session, graph buffer and positive capacity are required");
            }
            const std::string json = static_cast<xp_session*>(session)->value.Navigation().BuildGraphJson();
            preview_sdk::bridge::TextBuffer::Write(
                json,
                graphJson,
                static_cast<size_t>(capacity),
                "Navigation graph buffer is too small");
            return 1;
        } catch (const std::exception& error) {
            preview_sdk::bridge::LastError() = error.what();
            return 0;
        }
    }

    int MetadataAbi::xp_navigate(
        void* session,
        const char* navigationRequestJson
    ) {
        try {
            preview_sdk::bridge::LastError().clear();
            if (session == nullptr || navigationRequestJson == nullptr) {
                throw std::invalid_argument("Session and navigation request are required");
            }
            const std::vector<std::string> ids = preview_sdk::session::PreviewSessionBase::ParseNavigationTransitionIds(navigationRequestJson);
            std::vector<std::string_view> transitionIds;
            transitionIds.reserve(ids.size());
            for (const std::string& id : ids) {
                transitionIds.push_back(id);
            }
            if (!static_cast<xp_session*>(session)->value.Navigation().Navigate(
                transitionIds,
                preview_sdk::bridge::LastError())) {
                return 0;
            }
            return 1;
        } catch (const std::exception& error) {
            preview_sdk::bridge::LastError() = error.what();
            return 0;
        }
    }
} // namespace preview_sdk::abi