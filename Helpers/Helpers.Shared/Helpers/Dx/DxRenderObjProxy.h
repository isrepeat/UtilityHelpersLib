#pragma once
#include <Helpers/common.h>
#include "ISwapChainPanel.h"
#include "DxRenderObjBase.h"

namespace HELPERS_NS {
	namespace Dx {
		namespace details {
			class DxRenderObjProxy : public DxRenderObjWrapper<DxRenderObjDefaultData> {
			public:
				DxRenderObjProxy(
					Microsoft::WRL::ComPtr<HELPERS_NS::Dx::ISwapChainPanel> swapChainPanel,
					DXGI_FORMAT textureFormat = DXGI_FORMAT_B8G8R8A8_UNORM);

				void CreateWindowSizeDependentResources() override;
				void ReleaseDeviceDependentResources() override;

			private:
				std::unique_ptr<DxRenderObjDefaultData> CreateObjectData(DxDeviceSafeObj* dxDeviceSafeObj);
				void CreateTexture();

			private:
				Microsoft::WRL::ComPtr<HELPERS_NS::Dx::ISwapChainPanel> swapChainPanel;
				DXGI_FORMAT textureFormat;
			};
		}
	}
}