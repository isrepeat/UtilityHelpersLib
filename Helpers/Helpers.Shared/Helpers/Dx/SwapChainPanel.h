#pragma once
#include <Helpers/Dx/DxDevice.h>
#include <Helpers/Callback.hpp>
#include <Unknwn.h>

namespace HELPERS_NS {
	namespace Dx {
		// Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
		interface IDeviceNotify {
			virtual void OnDeviceLost() = 0;
			virtual void OnDeviceRestored() = 0;
		};

		enum DisplayOrientations : unsigned int {
			None = 0,
			Landscape = 0x1,
			Portrait = 0x2,
			LandscapeFlipped = 0x4,
			PortraitFlipped = 0x8,
		};


		// [Guid("B34F96A3-8A71-42D3-BE89-C09777D1BB99")]
		MIDL_INTERFACE("B34F96A3-8A71-42D3-BE89-C09777D1BB99")
		ISwapChainPanel : public IUnknown{
		public:
			virtual H::Dx::DxDeviceSafeObj* STDMETHODCALLTYPE GetDxDevice() = 0;

			virtual void STDMETHODCALLTYPE InitSwapChainPanelInfo(
				H::Size_f logicalSize,
				DisplayOrientations nativeOrientation,
				DisplayOrientations currentOrientation,
				float compositionScaleX,
				float compositionScaleY,
				float dpi
			) = 0;


			virtual void STDMETHODCALLTYPE SetLogicalSize(H::Size_f logicalSize) = 0;
			virtual void STDMETHODCALLTYPE SetNativeOrientation(DisplayOrientations nativeOrientation) = 0;
			virtual void STDMETHODCALLTYPE SetCurrentOrientation(DisplayOrientations currentOrientation) = 0;
			virtual void STDMETHODCALLTYPE SetDpi(float dpi) = 0;
			virtual void STDMETHODCALLTYPE SetCompositionScale(float compositionScaleX, float compositionScaleY) = 0;
			virtual void STDMETHODCALLTYPE SetRenderResolutionScale(float resolutionScale) = 0;
			virtual void STDMETHODCALLTYPE ValidateDevice() = 0;
			virtual void STDMETHODCALLTYPE HandleDeviceLost() = 0;
			virtual void STDMETHODCALLTYPE RegisterDeviceNotify(IDeviceNotify* deviceNotify) = 0;
			virtual void STDMETHODCALLTYPE Trim() = 0;
			virtual void STDMETHODCALLTYPE Present() = 0;

			// The size of the render target, in pixels.
			virtual H::Size_f STDMETHODCALLTYPE GetOutputSize() const = 0;

			// The size of the render target, in dips.
			virtual H::Size_f STDMETHODCALLTYPE GetLogicalSize() const = 0;
			virtual H::Size_f STDMETHODCALLTYPE GetRenderTargetSize() const = 0;
			virtual float STDMETHODCALLTYPE GetDpi() const = 0;

			// D3D Accessors.
			virtual IDXGISwapChain3* STDMETHODCALLTYPE GetSwapChain() const = 0;
			virtual ID3D11RenderTargetView1* STDMETHODCALLTYPE GetBackBufferRenderTargetView() const = 0;
			virtual ID3D11DepthStencilView* STDMETHODCALLTYPE GetDepthStencilView() const = 0;
			virtual D3D11_VIEWPORT STDMETHODCALLTYPE GetScreenViewport() const = 0;
			virtual DirectX::XMFLOAT4X4	STDMETHODCALLTYPE GetOrientationTransform3D() const = 0;

			// D2D Accessors.
			virtual ID2D1Bitmap1* STDMETHODCALLTYPE GetD2DTargetBitmap() const = 0;
			virtual D2D1::Matrix3x2F STDMETHODCALLTYPE GetOrientationTransform2D() const = 0;
		};
		

		// Controls all the DirectX device resources.
		class SwapChainPanel : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<
			Microsoft::WRL::RuntimeClassType::ClassicCom>,
			ISwapChainPanel>
		{
		public:
			SwapChainPanel(Callback<void, IDXGISwapChain3*> swapChainCreateFn);
			~SwapChainPanel();

			H::Dx::DxDeviceSafeObj* STDMETHODCALLTYPE GetDxDevice() override;

			void STDMETHODCALLTYPE InitSwapChainPanelInfo(
				H::Size_f logicalSize,
				DisplayOrientations nativeOrientation,
				DisplayOrientations currentOrientation,
				float compositionScaleX,
				float compositionScaleY,
				float dpi
			) override;

			void STDMETHODCALLTYPE SetLogicalSize(H::Size_f logicalSize) override;
			void STDMETHODCALLTYPE SetNativeOrientation(DisplayOrientations nativeOrientation) override;
			void STDMETHODCALLTYPE SetCurrentOrientation(DisplayOrientations currentOrientation) override;
			void STDMETHODCALLTYPE SetDpi(float dpi) override;
			void STDMETHODCALLTYPE SetCompositionScale(float compositionScaleX, float compositionScaleY) override;
			void STDMETHODCALLTYPE SetRenderResolutionScale(float resolutionScale) override;
			void STDMETHODCALLTYPE ValidateDevice() override;
			void STDMETHODCALLTYPE HandleDeviceLost() override;
			void STDMETHODCALLTYPE RegisterDeviceNotify(IDeviceNotify* deviceNotify) override;
			void STDMETHODCALLTYPE Trim() override;
			void STDMETHODCALLTYPE Present() override;

			// The size of the render target, in pixels.
			H::Size_f STDMETHODCALLTYPE GetOutputSize() const override  { return m_outputSize; }

			// The size of the render target, in dips.
			H::Size_f STDMETHODCALLTYPE GetLogicalSize() const override  { return m_logicalSize; }
			H::Size_f STDMETHODCALLTYPE GetRenderTargetSize() const override { return m_d3dRenderTargetSize; }
			float STDMETHODCALLTYPE GetDpi() const override { return m_effectiveDpi; }

			// D3D Accessors.
			IDXGISwapChain3* STDMETHODCALLTYPE GetSwapChain() const override { return m_swapChain.Get(); }
			ID3D11RenderTargetView1* STDMETHODCALLTYPE GetBackBufferRenderTargetView() const override { return m_d3dRenderTargetView.Get(); }
			ID3D11DepthStencilView* STDMETHODCALLTYPE GetDepthStencilView() const override { return m_d3dDepthStencilView.Get(); }
			D3D11_VIEWPORT STDMETHODCALLTYPE GetScreenViewport() const override { return m_screenViewport; }
			DirectX::XMFLOAT4X4	STDMETHODCALLTYPE GetOrientationTransform3D() const override { return m_orientationTransform3D; }

			// D2D Accessors.
			ID2D1Bitmap1* STDMETHODCALLTYPE GetD2DTargetBitmap() const override { return m_d2dTargetBitmap.Get(); }
			D2D1::Matrix3x2F STDMETHODCALLTYPE GetOrientationTransform2D() const override { return m_orientationTransform2D; }

		private:
			void CreateWindowSizeDependentResources();
			void UpdateRenderTargetSize();
			DXGI_MODE_ROTATION ComputeDisplayRotation();


		private:
			Callback<void, IDXGISwapChain3*> swapChainCreateFn;
			
			// Direct3D objects.
			H::Dx::DxDeviceSafeObj dxDeviceSafeObj;
			Microsoft::WRL::ComPtr<IDXGISwapChain3>	m_swapChain;

			// Direct3D rendering objects. Required for 3D.
			Microsoft::WRL::ComPtr<ID3D11RenderTargetView1>	m_d3dRenderTargetView;
			Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_d3dDepthStencilView;
			D3D11_VIEWPORT m_screenViewport;

			// Direct2D drawing components.
			Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_d2dTargetBitmap;


			// Cached device properties.
			H::Size_f m_d3dRenderTargetSize;
			H::Size_f m_outputSize;
			H::Size_f m_logicalSize;
			DisplayOrientations m_nativeOrientation;
			DisplayOrientations m_currentOrientation;
			float m_dpi;
			float m_compositionScaleX;
			float m_compositionScaleY;

			// Variables that take into account whether the app supports high resolution screens or not.
			float m_effectiveDpi;
			float m_effectiveCompositionScaleX;
			float m_effectiveCompositionScaleY;
			float m_resolutionScale;

			// Transforms used for display orientation.
			D2D1::Matrix3x2F m_orientationTransform2D;
			DirectX::XMFLOAT4X4	m_orientationTransform3D;

			// The IDeviceNotify can be held directly as it owns the DeviceResources.
			IDeviceNotify* m_deviceNotify;
		};
	}
}