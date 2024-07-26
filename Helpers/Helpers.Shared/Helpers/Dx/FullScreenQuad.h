#pragma once
#include <Helpers/common.h>
#include "DxRenderObj.h"
#include "DxDevice.h"

namespace HELPERS_NS {
	namespace Dx {
		namespace details {
			class FullScreenQuad {
			public:
				FullScreenQuad(DxDeviceSafeObj* dxDeviceSafeObj);
				~FullScreenQuad() = default;

				void Draw(
					Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureSRV,
					std::function<void __cdecl()> setCustomState = nullptr);

				void Draw(
					Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureSRV,
					DxRenderObjBase* outterRenderObj,
					std::function<void __cdecl()> setCustomState);

			private:
				DxDeviceSafeObj* dxDeviceSafeObj;
				DxRenderObj defaultRenderObj;
			};
		}
	}
}