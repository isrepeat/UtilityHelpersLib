#include "DxHelpers.h"
#include <Helpers/System.h>
#include <dxgi1_3.h>

namespace HELPERS_NS {
    namespace Dx {
        namespace details {
            DXGI_ADAPTER_DESC1 GetAdapterDescription(const Microsoft::WRL::ComPtr<IDXGIAdapter>& dxgiAdapter) {
                Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgiAdapter1;
                dxgiAdapter.As(&dxgiAdapter1);

                DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
                HRESULT hr = dxgiAdapter1->GetDesc1(&dxgiAdapterDesc);

                if (SUCCEEDED(hr))
                    return dxgiAdapterDesc;

                return {};
            }
        }


        EnumAdaptersState::EnumAdaptersState()
            : idx(0)
        {
            HRESULT hr = S_OK;
            hr = CreateDXGIFactory1(IID_PPV_ARGS(this->dxgiFactory.GetAddressOf()));
            H::System::ThrowIfFailed(hr);
        }

        Adapter EnumAdaptersState::Next() {
            HRESULT hr = S_OK;
            Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;

            hr = this->dxgiFactory->EnumAdapters(this->idx++, adapter.GetAddressOf());
            if (hr == DXGI_ERROR_NOT_FOUND) {
                return { nullptr };
            }
            H::System::ThrowIfFailed(hr);

            return Adapter{ adapter, details::GetAdapterDescription(adapter).Description };
        }

    }
}