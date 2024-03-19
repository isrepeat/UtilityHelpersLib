#include "DxSettings.h"
#include <functional>
#include <vector>
#include <string>
#include <atomic>

namespace HELPERS_NS {
    namespace Dx {
        DxSettings::DxSettings()
            : msaa{ false }
            , vsync{ true }
            , dxSettingsHandlers{ std::make_unique<DxSettingsHandlers>() }
        {
            UpdateAdapters();
            currentAdapter = adapters.at(0);
        }


        void DxSettings::EnableMSAA(bool enabled) {
            msaa = enabled;
            if (dxSettingsHandlers->msaaChanged) {
                dxSettingsHandlers->msaaChanged();
            }
        }

        bool DxSettings::IsMSAAEnabled() {
            return msaa;
        }


        void DxSettings::EnableVSync(bool enabled) {
            vsync = enabled;
            if (dxSettingsHandlers->vsyncChanged) {
                dxSettingsHandlers->vsyncChanged();
            }
        }

        bool DxSettings::IsVSyncEnabled() {
            return vsync;
        }


        void DxSettings::SetCurrentAdapterByIdx(uint32_t idx) {
            if (idx >= adapters.size()) {
                return;
            }
            currentAdapter = adapters[idx];
            if (dxSettingsHandlers->currentAdapterChanged) {
                dxSettingsHandlers->currentAdapterChanged();
            }
        }

        Adapter DxSettings::GetCurrentAdapter() {
            return currentAdapter;
        }

 
        void DxSettings::UpdateAdapters() {
            // TODO: add mutex
            adapters.clear();
            EnumAdaptersState enumAdapters;

            while (auto adapter = enumAdapters.Next()) {
                adapters.push_back(adapter);
            }
            if (dxSettingsHandlers->adapersUpdated) {
                dxSettingsHandlers->adapersUpdated();
            }
        }

        std::vector<Adapter> DxSettings::GetAdapters() {
            return adapters;
        }
       
        std::unique_ptr<DxSettingsHandlers>& DxSettings::GetDxSettingsHandlers() {
            return dxSettingsHandlers;
        }
    }
}