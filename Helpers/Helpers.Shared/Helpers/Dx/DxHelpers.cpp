#include "DxHelpers.h"
#include <Helpers/System.h>

namespace HELPERS_NS {
    namespace Dx {
        namespace details {
            DXGI_ADAPTER_DESC1 GetAdapterDescription(const Microsoft::WRL::ComPtr<IDXGIAdapter>& dxgiAdapter) {
                HRESULT hr = S_OK;

                Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgiAdapter1;
                hr = dxgiAdapter.As(&dxgiAdapter1);
                if (FAILED(hr))
                    return {};

                DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
                hr = dxgiAdapter1->GetDesc1(&dxgiAdapterDesc);

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

            auto adapterDesc = details::GetAdapterDescription(adapter);

            return Adapter{ adapter, adapterDesc.Description, this->idx - 1, adapterDesc.AdapterLuid };
        }
    }
}