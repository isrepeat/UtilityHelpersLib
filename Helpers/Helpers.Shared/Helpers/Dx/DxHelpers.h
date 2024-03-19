#pragma once
#include <Helpers/common.h>
#include <vector>
#include <string>
#include <dxgi.h>
#include <wrl.h>

namespace HELPERS_NS {
    namespace Dx {
        struct Adapter {
            Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
            std::wstring description;
            uint32_t idx;

            operator bool() {
                return static_cast<bool>(dxgiAdapter);
            }
        };

        class EnumAdaptersState {
        public:
            EnumAdaptersState();
            Adapter Next();

        private:
            uint32_t idx;
            Microsoft::WRL::ComPtr<IDXGIFactory1> dxgiFactory;
        };
    }
}