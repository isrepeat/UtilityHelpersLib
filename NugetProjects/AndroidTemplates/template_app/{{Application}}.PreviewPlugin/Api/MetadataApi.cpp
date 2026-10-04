#include "MetadataApi.h"

#include "../Bridge/PreviewPluginSdkTypes.h"
#include "../Bridge/Diagnostic.h"
#include "../Bridge/TextBuffer.h"
#include "PreviewPluginApi.h"
#include "SessionApi.h"

#include <string_view>
#include <stdexcept>
#include <string>
#include <vector>

namespace {{application}}::preview::api {
    using namespace AndroidAppPreviewerPluginSDK;

    //
    // API
    //
    const char* MetadataApi::xp_last_error(void) {
        return {{application}}::preview::api::PreviewPluginApi::LastError();
    }

    uint32_t MetadataApi::xp_get_abi_version(void) {
        return {{application}}::preview::api::PreviewPluginApi::AbiVersion();
    }

    int MetadataApi::xp_get_plugin_info(
        char* pluginInfoJson,
        int capacity
    ) {
        try {
            {{application}}::preview::bridge::LastError().clear();
            return {{application}}::preview::api::PreviewPluginApi::WritePluginInfo(pluginInfoJson, capacity) ? 1 : 0;
        } catch (const std::exception& error) {
            {{application}}::preview::bridge::LastError() = error.what();
            return 0;
        }
    }

    int MetadataApi::xp_get_initial_page_id(
        void* session,
        char* pageId,
        int capacity
    ) {
        return {{application}}::preview::api::SessionApi::xp_session_current_page(static_cast<xp_session*>(session), pageId, capacity);
    }

    int MetadataApi::xp_get_navigation_graph(
        void* session,
        char* graphJson,
        int capacity
    ) {
        try {
            {{application}}::preview::bridge::LastError().clear();
            if (session == nullptr || graphJson == nullptr || capacity <= 0) {
                throw std::invalid_argument("Session, graph buffer and positive capacity are required");
            }
            const std::string json = static_cast<xp_session*>(session)->value.Navigation().BuildGraphJson();
            {{application}}::preview::bridge::TextBuffer::Write(
                json,
                graphJson,
                static_cast<size_t>(capacity),
                "Navigation graph buffer is too small");
            return 1;
        } catch (const std::exception& error) {
            {{application}}::preview::bridge::LastError() = error.what();
            return 0;
        }
    }

    int MetadataApi::xp_navigate(
        void* session,
        const char* navigationRequestJson
    ) {
        try {
            {{application}}::preview::bridge::LastError().clear();
            if (session == nullptr || navigationRequestJson == nullptr) {
                throw std::invalid_argument("Session and navigation request are required");
            }
            const std::vector<std::string> ids = {{application}}::preview::session::PreviewSession::ParseNavigationTransitionIds(navigationRequestJson);
            std::vector<std::string_view> transitionIds;
            transitionIds.reserve(ids.size());
            for (const std::string& id : ids) {
                transitionIds.push_back(id);
            }
            if (!static_cast<xp_session*>(session)->value.Navigation().Navigate(
                transitionIds,
                {{application}}::preview::bridge::LastError())) {
                return 0;
            }
            return 1;
        } catch (const std::exception& error) {
            {{application}}::preview::bridge::LastError() = error.what();
            return 0;
        }
    }
} // namespace {{application}}::preview::api