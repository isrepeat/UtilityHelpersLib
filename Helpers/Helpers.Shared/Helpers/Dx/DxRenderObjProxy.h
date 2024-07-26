#pragma once
#include <Helpers/common.h>
#include "ISwapChainPanel.h"
#include "DxRenderObj.h"

namespace HELPERS_NS {
	namespace Dx {
		namespace details {
			class DxRenderObjProxy : public DxRenderObjWrapper<DxRenderObj> {
			public:
				DxRenderObjProxy(Microsoft::WRL::ComPtr<HELPERS_NS::Dx::ISwapChainPanel> swapChainPanel);

				void CreateWindowSizeDependentResources() override;
				void ReleaseDeviceDependentResources() override;

			private:
				std::unique_ptr<DxRenderObj> CreateObject(DxDeviceSafeObj* dxDeviceSafeObj);
				void CreateTexture();

			private:
				Microsoft::WRL::ComPtr<HELPERS_NS::Dx::ISwapChainPanel> swapChainPanel;
			};
		}
	}
}