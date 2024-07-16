#include "pch.h"
#include "SwapChainPanel.h"
#include <Helpers/Dx/DxHelpers.h>
#include <Helpers/CallbackWinRT.hpp>
#include <Helpers/System.h>

#include <windows.ui.xaml.media.dxinterop.h>

namespace ScreenRotation {
	H::Dx::DisplayOrientations ToNative(Windows::Graphics::Display::DisplayOrientations displayOrientation) {
		switch (displayOrientation) {
		case Windows::Graphics::Display::DisplayOrientations::Landscape:
			return H::Dx::DisplayOrientations::Landscape;

		case Windows::Graphics::Display::DisplayOrientations::Portrait:
			return H::Dx::DisplayOrientations::Portrait;

		case Windows::Graphics::Display::DisplayOrientations::LandscapeFlipped:
			return H::Dx::DisplayOrientations::LandscapeFlipped;

		case Windows::Graphics::Display::DisplayOrientations::PortraitFlipped:
			return H::Dx::DisplayOrientations::PortraitFlipped;
		}
	}
};

namespace Helpers {
	namespace WinRt {
		namespace Dx {
			SwapChainPanel::SwapChainPanel()
				: swapChainPanelNative{ this->CreateSwapChainPanelNative() }
			{
			}

			SwapChainPanel::~SwapChainPanel() {
			}

			// This method is called when the XAML control is created (or re-created).
			void SwapChainPanel::SetSwapChainPanel(Windows::UI::Xaml::Controls::SwapChainPanel^ swapChainPanelXaml) {
				this->swapChainPanelXaml = swapChainPanelXaml;
				
				auto currentDisplayInformation = Windows::Graphics::Display::DisplayInformation::GetForCurrentView();
				this->swapChainPanelNative->InitSwapChainPanelInfo(
					H::Size_f{ static_cast<float>(swapChainPanelXaml->ActualWidth), static_cast<float>(swapChainPanelXaml->ActualHeight) },
					ScreenRotation::ToNative(currentDisplayInformation->NativeOrientation),
					ScreenRotation::ToNative(currentDisplayInformation->CurrentOrientation),
					swapChainPanelXaml->CompositionScaleX,
					swapChainPanelXaml->CompositionScaleY,
					currentDisplayInformation->LogicalDpi
				);
			}

			// This method is called in the event handler for the SizeChanged event.
			void SwapChainPanel::SetLogicalSize(Windows::Foundation::Size logicalSize) {
				this->swapChainPanelNative->SetLogicalSize(H::Size_f{ logicalSize.Width, logicalSize.Height });
			}

			// This method is called in the event handler for the DpiChanged event.
			void SwapChainPanel::SetDpi(float dpi) {
				this->swapChainPanelNative->SetDpi(dpi);
			}

			// This method is called in the event handler for the OrientationChanged event.
			void SwapChainPanel::SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation) {
				this->swapChainPanelNative->SetCurrentOrientation(ScreenRotation::ToNative(currentOrientation));
			}

			// This method is called in the event handler for the CompositionScaleChanged event.
			void SwapChainPanel::SetCompositionScale(float compositionScaleX, float compositionScaleY) {
				this->swapChainPanelNative->SetCompositionScale(compositionScaleX, compositionScaleY);
			}

			// This method is called in the event handler for the DisplayContentsInvalidated event.
			void SwapChainPanel::ValidateDevice() {
				this->swapChainPanelNative->ValidateDevice();
			}

			// Recreate all device resources and set them back to the current state.
			void SwapChainPanel::HandleDeviceLost() {
				assert(false);
				// TODO: implement
			}



			// Call this method when the app suspends. It provides a hint to the driver that the app 
			// is entering an idle state and that temporary buffers can be reclaimed for use by other apps.
			void SwapChainPanel::Trim() {
				this->swapChainPanelNative->Trim();
			}

			// Present the contents of the swap chain to the screen.
			void SwapChainPanel::Present() {
				this->swapChainPanelNative->Present();
			}

			//Platform::Object^ SwapChainPanel::GetSwapChainNative() {
			//	Platform::Object^ swapChainPanelRefObj = nullptr;
			//	{
			//		//auto swapChainPanelCom = Microsoft::WRL::Make<H::UnknownValue<std::shared_ptr<H::Dx::SwapChainPanel>>>();
			//		auto swapChainPanelCom = Microsoft::WRL::Make<H::UnknownValue<H::Dx::SwapChainPanel*>>();
			//		auto swapChainPanelPtr = this->swapChainPanel.get();
			//		swapChainPanelCom->SetValue(swapChainPanelPtr);
			//		swapChainPanelRefObj = reinterpret_cast<Platform::Object^>(swapChainPanelCom.Get());
			//	}
			//	return swapChainPanelRefObj;
			//}

			//Platform::Object^ SwapChainPanel::GetSwapChainNative() {
			//	auto unkVal = Microsoft::WRL::Make<H::UnknownValue<H::Dx::SwapChainPanel*>>();
			//	unkVal->SetValue(this->swapChainPanel.get());

			//	Microsoft::WRL::ComPtr<IInspectable> insp;
			//	unkVal.As(&insp);

			//	Platform::Object^ obj = reinterpret_cast<Platform::Object^>(insp.Get());
			//	return obj;
			//}

			Platform::Object^ SwapChainPanel::GetSwapChainNative() {
				Platform::Object^ obj = reinterpret_cast<Platform::Object^>(this->swapChainPanelNative.Get());
				return obj;
			}

			// Register our DeviceNotify to be informed on device lost and creation.
			void SwapChainPanel::RegisterDeviceNotify(H::Dx::IDeviceNotify* deviceNotify) {
				this->swapChainPanelNative->RegisterDeviceNotify(deviceNotify);
			}

			Microsoft::WRL::ComPtr<H::Dx::ISwapChainPanel> SwapChainPanel::CreateSwapChainPanelNative() {
				H::Dx::SwapChainPanel::InitData swapChainPanelNativeInitData;
				swapChainPanelNativeInitData.environment = H::Dx::SwapChainPanel::InitData::Environment::UWP;
				swapChainPanelNativeInitData.creatSwapChainPannelDxgiFn = MakeWinRTCallback(this, &SwapChainPanel::CreateSwapChainPanelDxgi);
				return Microsoft::WRL::Make<H::Dx::SwapChainPanel>(swapChainPanelNativeInitData);
			}


			void SwapChainPanel::CreateSwapChainPanelDxgi(SwapChainPanel^ _this, IDXGISwapChain3* swapChainPanelDxgi) {
				// Associate swap chain with SwapChainPanel
				// UI changes will need to be dispatched back to the UI thread
				_this->swapChainPanelXaml->Dispatcher->RunAsync(
					Windows::UI::Core::CoreDispatcherPriority::High, ref new Windows::UI::Core::DispatchedHandler([=] {
						HRESULT hr = S_OK;

						// Get backing native interface for SwapChainPanel
						Microsoft::WRL::ComPtr<ISwapChainPanelNative> swapChainPanelNative;
						hr = reinterpret_cast<IUnknown*>(_this->swapChainPanelXaml)->QueryInterface(IID_PPV_ARGS(swapChainPanelNative.GetAddressOf()));
						H::System::ThrowIfFailed(hr);

						hr = swapChainPanelNative->SetSwapChain(swapChainPanelDxgi);
						H::System::ThrowIfFailed(hr);
						},
						Platform::CallbackContext::Any));
			}
		}
	}
}