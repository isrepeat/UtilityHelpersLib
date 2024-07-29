#pragma once
#include <Helpers/common.h>
#include <Helpers/Signal.h>
#include "DxHelpers.h"
#include <functional>
#include <vector>
#include <string>
#include <atomic>
#include <memory>

namespace HELPERS_NS {
    namespace Dx {
        struct DxSettingsHandlers {
            HELPERS_NS::Signal<void()> msaaChanged;
            HELPERS_NS::Signal<void()> vsyncChanged;
            HELPERS_NS::Signal<void()> adapersUpdated;
            HELPERS_NS::Signal<void()> currentAdapterChanged;
        };

        class DxSettings {
        public:
            DxSettings();

            void EnableMSAA(bool enabled);
            bool IsMSAAEnabled();

            void EnableVSync(bool enabled);
            bool IsVSyncEnabled();

            void SetCurrentAdapterByIdx(uint32_t idx);
            Adapter GetCurrentAdapter();

            void UpdateAdapters();
            std::vector<Adapter> GetAdapters();

            std::unique_ptr<DxSettingsHandlers>& GetDxSettingsHandlers();

        private:
            std::atomic<bool> msaa;
            std::atomic<bool> vsync;
            std::vector<Adapter> adapters;
            Adapter currentAdapter;

            std::unique_ptr<DxSettingsHandlers> dxSettingsHandlers;
        };
    }
}