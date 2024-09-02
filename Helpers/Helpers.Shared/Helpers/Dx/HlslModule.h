#pragma once
#include "Helpers/common.h"
#include "DxConstantBuffer.h"
#include <d3dcompiler.h>

namespace HELPERS_NS {
	namespace Dx {
		enum class Slot {
			_Return = D3D_RETURN_PARAMETER_INDEX,
			_0 = 0,
			_1,
			_2,
			_3,
		};

		struct HlslFunction {
			struct ParamMatch {
				Slot from;
				Slot to;
			};
			std::string functionName;
			std::vector<ParamMatch> paramsMatch;
		};

		struct HlslModule {
			struct BindResource {
				uint32_t srcSlot = 0;
				uint32_t dstSlot = 0;
				uint32_t count = 0;
			};
			struct BindSampler {
				uint32_t srcSlot = 0;
				uint32_t dstSlot = 0;
				uint32_t count = 0;
			};
			struct BindConstantBuffer {
				std::shared_ptr<DxConstantBufferBase> dxConstantBuffer;
				uint32_t srcSlot = 0;
				uint32_t dstSlot = 0;
				uint32_t cbDstOffset = 0;
			};

			std::filesystem::path hlslFile;
			std::vector<HlslFunction> hlslFunctions;
			std::optional<BindResource> bindResource;
			std::optional<BindSampler> bindSampler;
			std::vector<BindConstantBuffer> bindConstantBuffers;
		};
	}
}