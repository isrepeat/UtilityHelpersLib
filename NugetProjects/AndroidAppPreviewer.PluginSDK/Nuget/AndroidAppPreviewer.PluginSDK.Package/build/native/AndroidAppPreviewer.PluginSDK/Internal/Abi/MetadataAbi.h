#pragma once
#include "../../AndroidAppPreviewerPlugin.h"

namespace preview_sdk::abi {
    using namespace AndroidAppPreviewerPluginSDK;

    class MetadataAbi final {
    public:
        static uint32_t xp_get_abi_version(void);
        static int xp_get_plugin_info(
            char* pluginInfoJson,
            int capacity
        );
        static int xp_get_initial_page_id(
            void* session,
            char* pageId,
            int capacity
        );
        static int xp_get_navigation_graph(
            void* session,
            char* graphJson,
            int capacity
        );
        static int xp_navigate(
            void* session,
            const char* navigationRequestJson
        );
        static const char* xp_last_error(void);
    };
}