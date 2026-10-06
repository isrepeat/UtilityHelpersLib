#pragma once
#include "PreviewSessionBase.h"

#include <memory>
#include <string>

namespace preview_sdk {
    // Реализуются приложением; общий runtime не зависит от его конкретных классов.
    std::unique_ptr<session::PreviewSessionBase> CreatePreviewSession(int width, int height);
    std::string PluginInfoJson();
}