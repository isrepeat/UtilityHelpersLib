#pragma once
#include <Helpers/common.h>
#include "DxRenderObjBase.h"
#include "ISwapChainPanel.h"
#include "DxDevice.h"

namespace HELPERS_NS {
	namespace Dx {
		namespace details {
			struct DxRenderObjHDRData : DxRenderObjBaseData {
				Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShaderHDR;
				Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShaderToneMap;

				void Reset() override {
					this->DxRenderObjBaseData::Reset();
					this->pixelShaderHDR.Reset();
					this->pixelShaderToneMap.Reset();
				}
			};
			
			class DxRenderObjHDR : public DxRenderObjWrapper<DxRenderObjHDRData> {
			public:
				// Shader filepathes are relative to .exe
				struct Params {
					std::filesystem::path vertexShader;
					std::filesystem::path pixelShaderHDR;
					std::filesystem::path pixelShaderToneMap;
				};

				DxRenderObjHDR(
					Microsoft::WRL::ComPtr<HELPERS_NS::Dx::ISwapChainPanel> swapChainPanel,
					Params params);

				DxRenderObjHDR(
					DxDeviceSafeObj* dxDeviceSafeObj,
					Params params);

				void CreateWindowSizeDependentResources() override;
				void ReleaseDeviceDependentResources() override;

				void CreateTextureHDR(HELPERS_NS::Size size);
			private:
				std::unique_ptr<DxRenderObjHDRData> CreateObjectData(
					HELPERS_NS::Dx::DxDeviceSafeObj* dxDeviceSafeObj,
					Params params);

				//void CreateTextureHDR();

			private:
				Microsoft::WRL::ComPtr<HELPERS_NS::Dx::ISwapChainPanel> swapChainPanel;
			};
		}
	}
}