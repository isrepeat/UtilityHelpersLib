#pragma once
#include <Helpers/Dx/SwapChainPanel.h>
#include <Helpers/Com/UnknownValue.h>

namespace Helpers {
	namespace WinRt {
		namespace Dx {
			
			// Controls all the DirectX device resources.
			public ref class SwapChainPanel sealed {
			public:
				SwapChainPanel();
				virtual ~SwapChainPanel();

				void SetSwapChainPanel(Windows::UI::Xaml::Controls::SwapChainPanel^ panel);
				void SetLogicalSize(Windows::Foundation::Size logicalSize);
				void SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation);
				void SetDpi(float dpi);
				void SetCompositionScale(float compositionScaleX, float compositionScaleY);
				void ValidateDevice();
				void HandleDeviceLost();
				void Trim();
				void Present();

				Platform::Object^ GetSwapChainNative();

			internal:
				void RegisterDeviceNotify(H::Dx::IDeviceNotify* deviceNotify);

			private:
				Microsoft::WRL::ComPtr<H::Dx::ISwapChainPanel> CreateSwapChainPanelNative();
				static void CreateSwapChainPanelDxgi(SwapChainPanel^ _this, IDXGISwapChain3* dxgiSwapChainPanel);

			private:
				Microsoft::WRL::ComPtr<H::Dx::ISwapChainPanel> swapChainPanelNative;
				Windows::UI::Xaml::Controls::SwapChainPanel^ swapChainPanelXaml;
			};
		}
	}
}