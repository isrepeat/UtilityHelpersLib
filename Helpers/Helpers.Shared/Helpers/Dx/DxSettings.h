#pragma once
#include <Helpers/common.h>
#include "DxHelpers.h"
#include <functional>
#include <vector>
#include <string>
#include <atomic>

namespace HELPERS_NS {
    namespace Dx {
        struct DxSettingsHandlers {
            std::function<void()> msaaChanged;
            std::function<void()> vsyncChanged;
            std::function<void()> adapersUpdated;
            std::function<void()> currentAdapterChanged;
        };

        class DxSettings {
        public:
            DxSettings();

            //void SetMSAAChangedCallback(std::function<void()> msaaChangedCallback);
            void EnableMSAA(bool enabled);
            bool IsMSAAEnabled();

            //void SetVSyncChangedCallback(std::function<void()> vsyncChangedCallback);
            void EnableVSync(bool enabled);
            bool IsVSyncEnabled();

            //void SetCurrentAdapterChangedCallback(std::function<void()> currentAdapterChangedCallback);
            void SetCurrentAdapterByIdx(uint32_t idx);
            Adapter GetCurrentAdapter();

            //void SetAdaptersUpdatedCallback(std::function<void()> adapersUpdatedCallback);
            void UpdateAdapters();
            std::vector<Adapter> GetAdapters();

            DxSettingsHandlers& GetDxSettingsHandlers();

        private:
            std::atomic<bool> msaa;
            std::atomic<bool> vsync;
            std::vector<Adapter> adapters;
            Adapter currentAdapter;

            DxSettingsHandlers dxSettingsHandlers;
        };
    }
}