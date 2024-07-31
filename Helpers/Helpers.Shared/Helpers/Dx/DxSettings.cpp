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
            this->currentAdapter = adapters.at(0);
        }


        void DxSettings::EnableMSAA(bool enabled) {
            this->msaa = enabled;
            if (this->dxSettingsHandlers->msaaChanged) {
                this->dxSettingsHandlers->msaaChanged();
            }
        }

        bool DxSettings::IsMSAAEnabled() {
            return this->msaa;
        }


        void DxSettings::EnableVSync(bool enabled) {
            this->vsync = enabled;
            if (this->dxSettingsHandlers->vsyncChanged) {
                this->dxSettingsHandlers->vsyncChanged();
            }
        }

        bool DxSettings::IsVSyncEnabled() {
            return this->vsync;
        }


        void DxSettings::SetCurrentAdapterByIdx(uint32_t idx) {
            if (idx >= this->adapters.size()) {
                return;
            }
            this->currentAdapter = this->adapters[idx];
            if (this->dxSettingsHandlers->currentAdapterChanged) {
                this->dxSettingsHandlers->currentAdapterChanged();
            }
        }

        Adapter DxSettings::GetCurrentAdapter() {
            return this->currentAdapter;
        }

 
        void DxSettings::UpdateAdapters() {
            // TODO: add mutex
            this->adapters.clear();
            EnumAdaptersState enumAdapters;

            while (auto adapter = enumAdapters.Next()) {
                this->adapters.push_back(adapter);
            }
            if (this->dxSettingsHandlers->adapersUpdated) {
                this->dxSettingsHandlers->adapersUpdated();
            }
        }

        std::vector<Adapter> DxSettings::GetAdapters() {
            return this->adapters;
        }
       
        std::unique_ptr<DxSettingsHandlers>& DxSettings::GetDxSettingsHandlers() {
            return this->dxSettingsHandlers;
        }
    }
}