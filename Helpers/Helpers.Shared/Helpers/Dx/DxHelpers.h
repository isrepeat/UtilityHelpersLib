#pragma once
#include <Helpers/common.h>
#include <vector>
#include <string>
#include <dxgi.h>
#include <wrl.h>

namespace HELPERS_NS {
    namespace Dx {
        class Adapter {
        public:
            Adapter();
            Adapter(Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter, uint32_t idx);

            Microsoft::WRL::ComPtr<IDXGIAdapter> GetDXGIAdapter() const;
            uint32_t GetIndex() const;

            DXGI_ADAPTER_DESC1 GetDXGIDescription() const;
            std::wstring GetDescription() const;
            LUID GetAdapterLUID() const;

            operator bool() const {
                return static_cast<bool>(dxgiAdapter);
            }

        private:
            Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
            uint32_t idx;
            DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
        };


        class Output {
        public:
            Output();
            Output(Microsoft::WRL::ComPtr<IDXGIOutput> output, uint32_t idx, Adapter adapter);

            Microsoft::WRL::ComPtr<IDXGIOutput> GetDXGIOutput() const;
            uint32_t GetIndex() const;
            Adapter GetAdapter() const;
            
            DXGI_OUTPUT_DESC GetDXGIDescription() const;
            
            operator bool() const {
                return static_cast<bool>(dxgiOutput);
            }

        private:
            Microsoft::WRL::ComPtr<IDXGIOutput> dxgiOutput;
            uint32_t idx;
            Adapter adapter;
            DXGI_OUTPUT_DESC dxgiOutputDesc;
        };
        

        class EnumAdaptersState {
        public:
            EnumAdaptersState();
            Adapter Next();

        private:
            uint32_t idx;
            Microsoft::WRL::ComPtr<IDXGIFactory1> dxgiFactory;
        };


        class EnumOutputsState {
        public:
            EnumOutputsState(Adapter adapter);
            Output Next();

        private:
            uint32_t idx;
            Adapter adapter;
        };


        void LogDeviceInfo();
    }
}