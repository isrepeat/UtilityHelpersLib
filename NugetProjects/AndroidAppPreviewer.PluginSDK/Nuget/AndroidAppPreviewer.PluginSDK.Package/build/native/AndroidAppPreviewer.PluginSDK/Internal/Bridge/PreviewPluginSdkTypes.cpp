#include "PreviewPluginSdkTypes.h"

#include "../../Session/PluginDefinition.h"

namespace AndroidAppPreviewerPluginSDK {
    xp_angle_surface::xp_angle_surface(
        int width,
        int height,
        const char* fontPath,
        const char* resourceRoot)
        : width(width)
        , height(height)
        , value(width, height, fontPath, resourceRoot) {
    }

    xp_session::xp_session(int width, int height)
        : previewSession(preview_sdk::CreatePreviewSession(width, height))
        , value(*this->previewSession) {
    }
}