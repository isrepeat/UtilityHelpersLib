#include "DxHelpers.h"
#include <Helpers/System.h>

namespace HELPERS_NS {
    namespace Dx {
        namespace details {
            DXGI_ADAPTER_DESC1 GetAdapterDescription(const Microsoft::WRL::ComPtr<IDXGIAdapter>& dxgiAdapter) {
                HRESULT hr = S_OK;
                if (!dxgiAdapter)
                    return {};

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

            DXGI_OUTPUT_DESC GetOutputDescription(const Microsoft::WRL::ComPtr<IDXGIOutput>& dxgiOutput) {
                HRESULT hr = S_OK;
                if (!dxgiOutput)
                    return {};

                DXGI_OUTPUT_DESC dxgiOutputDesc;
                hr = dxgiOutput->GetDesc(&dxgiOutputDesc);

                if (SUCCEEDED(hr))
                    return dxgiOutputDesc;

                return {};
            }
        }


        Adapter::Adapter()
            : Adapter(nullptr, -1)
        {}
        Adapter::Adapter(Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter, uint32_t idx)
            : dxgiAdapter{ dxgiAdapter }
            , idx{ idx }
            , dxgiAdapterDesc1{ details::GetAdapterDescription(dxgiAdapter) }
        {}

        Microsoft::WRL::ComPtr<IDXGIAdapter> Adapter::GetDXGIAdapter() const {
            return dxgiAdapter;
        }
        uint32_t Adapter::GetIndex() const {
            return idx;
        }
        DXGI_ADAPTER_DESC1 Adapter::GetDXGIDescription() const {
            return dxgiAdapterDesc1;
        }
        std::wstring Adapter::GetDescription() const {
            return dxgiAdapterDesc1.Description;
        }
        LUID Adapter::GetAdapterLUID() const {
            return dxgiAdapterDesc1.AdapterLuid;
        }


        Output::Output()
            : Output(nullptr, -1, Adapter{})
        {}
        Output::Output(Microsoft::WRL::ComPtr<IDXGIOutput> output, uint32_t idx, Adapter adapter)
            : dxgiOutput{ output }
            , idx{ idx }
            , adapter{ adapter }
            , dxgiOutputDesc{ details::GetOutputDescription(dxgiOutput) }
        {}

        Microsoft::WRL::ComPtr<IDXGIOutput> Output::GetDXGIOutput() const {
            return dxgiOutput;
        }
        uint32_t Output::GetIndex() const {
            return idx;
        }
        Adapter Output::GetAdapter() const {
            return adapter;
        }
        DXGI_OUTPUT_DESC Output::GetDXGIDescription() const {
            return dxgiOutputDesc;
        }


        EnumAdaptersState::EnumAdaptersState()
            : idx(0)
        {
            HRESULT hr = S_OK;
            hr = CreateDXGIFactory1(IID_PPV_ARGS(this->dxgiFactory.GetAddressOf()));
            HELPERS_NS::System::ThrowIfFailed(hr);
        }

        Adapter EnumAdaptersState::Next() {
            HRESULT hr = S_OK;
            Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;

            hr = this->dxgiFactory->EnumAdapters(this->idx++, adapter.GetAddressOf());
            if (hr == DXGI_ERROR_NOT_FOUND) {
                return {};
            }
            HELPERS_NS::System::ThrowIfFailed(hr);
            return Adapter{ adapter, this->idx - 1 };
        }


        EnumOutputsState::EnumOutputsState(Adapter adapter)
            : idx(0)
            , adapter{ adapter }
        {}
        
        Output EnumOutputsState::Next() {
            HRESULT hr = S_OK;
            Microsoft::WRL::ComPtr<IDXGIOutput> output;

            hr = this->adapter.GetDXGIAdapter()->EnumOutputs(this->idx++, output.GetAddressOf());
            if (hr == DXGI_ERROR_NOT_FOUND) {
                return {};
            }
            HELPERS_NS::System::ThrowIfFailed(hr);
            return Output{ output, this->idx - 1, adapter };
        }


        void LogDeviceInfo() {
            std::vector<Adapter> adapters;
            std::vector<Output> outputs;

            EnumAdaptersState enumAdapters;
            while (auto adapter = enumAdapters.Next()) {
                adapters.push_back(adapter);

                EnumOutputsState enumOutputs(adapter);
                while (auto output = enumOutputs.Next()) {
                    outputs.push_back(output);
                }
            }

            LOG_DEBUG_D("Enum adapters:");
            for (auto& adapter : adapters) {
                LOG_DEBUG_D(L"[{}] Adapter = {}", adapter.GetIndex(), adapter.GetDescription());
            }

            LOG_DEBUG_D("");
            LOG_DEBUG_D("Enum outputs:");
            for (auto& output : outputs) {
                RECT rect = output.GetDXGIDescription().DesktopCoordinates;
                LOG_DEBUG_D(L"[{}] Output [{}]", output.GetIndex(), output.GetAdapter().GetDescription());
                LOG_DEBUG_D(L"     - device name = {}", output.GetDXGIDescription().DeviceName);
                LOG_DEBUG_D(L"     - desktop coords = {{LT({}, {}) RB({}, {})}} [{}x{}]", rect.left, rect.top, rect.right, rect.bottom, (rect.right - rect.left), (rect.bottom - rect.top));
            }
        }

        HELPERS_NS::Rational<double> GetRefreshRateForDXGIOutput(Microsoft::WRL::ComPtr<IDXGIOutput> dxgiOutput) {
            HRESULT hr = S_OK;

            if (dxgiOutput) {
                UINT num = 0;
                hr = dxgiOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, 0, &num, 0);
                HELPERS_NS::System::ThrowIfFailed(hr);

                std::vector<DXGI_MODE_DESC> modeDescs(num);
                hr = dxgiOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, 0, &num, modeDescs.data());
                HELPERS_NS::System::ThrowIfFailed(hr);

                // TODO: may be need return for current monitor resolution {width, height}
                if (modeDescs.size() > 0) {
                    auto refreshRate = modeDescs[0].RefreshRate;
                    return HELPERS_NS::Rational<double>{ refreshRate.Numerator, refreshRate.Denominator };
                }
            }
            return {0, 1};
        }


        // Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
        float ConvertDipsToPixels(float dips, float dpi) {
            static const float dipsPerInch = 96.0f;
            return floorf(dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
        }

#if defined(_DEBUG)
        // Check for SDK Layer support.
        bool SdkLayersAvailable() {
            HRESULT hr = D3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
                0,
                D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
                nullptr,                    // Any feature level will do.
                0,
                D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Microsoft Store apps.
                nullptr,                    // No need to keep the D3D device reference.
                nullptr,                    // No need to know the feature level.
                nullptr                     // No need to keep the D3D device context reference.
            );

            return SUCCEEDED(hr);
        }
#endif
    }
}