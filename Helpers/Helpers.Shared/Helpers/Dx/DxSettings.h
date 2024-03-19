#pragma once
#include <Helpers/common.h>
#include "DxHelpers.h"
#include <vector>
#include <string>
#include <atomic>

namespace HELPERS_NS {
    namespace Dx {
        class DxSettings {
        public:
            DxSettings()
                : msaa{ false }
                , vsync{ true }
            {}

            void EnableMSAA(bool enabled) {
                msaa = enabled;
            }
            bool IsMSAAEnabled() {
                return msaa;
            }


            void EnableVSync(bool enabled) {
                vsync = enabled;
            }
            bool IsVSyncEnabled() {
                return vsync;
            }


            void UpdateAdapters() {
                // TODO: add mutex
                adapters.clear();
                EnumAdaptersState enumAdapters;

                while (auto adapter = enumAdapters.Next()) {
                    adapters.push_back(adapter);
                }
            }
            std::vector<Adapter> GetAdapters() {                
                return adapters;
            }

        private:
            std::atomic<bool> msaa;
            std::atomic<bool> vsync;
            std::vector<Adapter> adapters;
        };
    }
}