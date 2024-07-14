#include "SwapChainPanel.h"
#include <Helpers/Dx/DxHelpers.h>
#include <Helpers/System.h>

#include <windows.ui.xaml.media.dxinterop.h>


namespace DisplayMetrics {
	// High resolution displays can require a lot of GPU and battery power to render.
	// High resolution phones, for example, may suffer from poor battery life if
	// games attempt to render at 60 frames per second at full fidelity.
	// The decision to render at full fidelity across all platforms and form factors
	// should be deliberate.
	static const bool SupportHighResolutions = false;

	// The default thresholds that define a "high resolution" display. If the thresholds
	// are exceeded and SupportHighResolutions is false, the dimensions will be scaled
	// by 50%.
	static const float DpiThreshold = 192.0f;		// 200% of standard desktop display.
	static const float WidthThreshold = 1920.0f;	// 1080p width.
	static const float HeightThreshold = 1080.0f;	// 1080p height.
};

// Constants used to calculate screen rotations.
namespace ScreenRotation {
	// 0-degree Z-rotation
	static const DirectX::XMFLOAT4X4 Rotation0(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	// 90-degree Z-rotation
	static const DirectX::XMFLOAT4X4 Rotation90(
		0.0f, 1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	// 180-degree Z-rotation
	static const DirectX::XMFLOAT4X4 Rotation180(
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	// 270-degree Z-rotation
	static const DirectX::XMFLOAT4X4 Rotation270(
		0.0f, -1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
};

namespace HELPERS_NS {
	namespace Dx {
		SwapChainPanel::SwapChainPanel(Environment environment, Callback<void, IDXGISwapChain3*> swapChainCreateFn, HWND hWnd)
			: environment{ environment }
			, swapChainCreateFn{ swapChainCreateFn }
			, hWnd{ hWnd }
			, dxDeviceSafeObj{ std::make_unique<H::Dx::details::DxVideoDeviceMF>() }
			, m_screenViewport{}
			, m_d3dRenderTargetSize{}
			, m_outputSize{}
			, m_logicalSize{}
			, m_nativeOrientation{ DXGI_MODE_ROTATION::DXGI_MODE_ROTATION_IDENTITY }
			, m_currentOrientation{ DXGI_MODE_ROTATION::DXGI_MODE_ROTATION_IDENTITY }
			, m_dpi{ -1.0f }
			, m_effectiveDpi{ -1.0f }
			, m_compositionScaleX{ 1.0f }
			, m_compositionScaleY{ 1.0f }
			, m_resolutionScale{ 1.0f }
			, m_deviceNotify{ nullptr }
		{
		}

		SwapChainPanel::~SwapChainPanel() {
		}

		H::Dx::DxDeviceSafeObj* SwapChainPanel::GetDxDevice() {
			return &this->dxDeviceSafeObj;
		}

		void SwapChainPanel::InitSwapChainPanelInfo(
			H::Size_f logicalSize,
			DisplayOrientations nativeOrientation,
			DisplayOrientations currentOrientation,
			float compositionScaleX,
			float compositionScaleY,
			float dpi
		)
		{
			auto dxDev = this->dxDeviceSafeObj.Lock();
			auto dxCtx = dxDev->LockContext();
			auto d2dCtx = dxCtx->D2D();

			this->m_logicalSize = logicalSize;
			this->m_nativeOrientation = nativeOrientation;
			this->m_currentOrientation = currentOrientation;
			this->m_compositionScaleX = compositionScaleX;
			this->m_compositionScaleY = compositionScaleY;
			this->m_dpi = dpi;

			d2dCtx->SetDpi(this->m_dpi, this->m_dpi);
			this->CreateWindowSizeDependentResources();
		}


		// These resources need to be recreated every time the window size is changed.
		void SwapChainPanel::CreateWindowSizeDependentResources() {
			HRESULT hr = S_OK;
			auto dxDev = this->dxDeviceSafeObj.Lock();
			auto d3dDevice = dxDev->GetD3DDevice();
			auto dxCtx = dxDev->LockContext();
			auto d2dCtx = dxCtx->D2D();
			auto d3dCtx = dxCtx->D3D();

			// Clear the previous window size specific context.
			ID3D11RenderTargetView* nullViews[] = { nullptr };
			d3dCtx->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
			m_d3dRenderTargetView = nullptr;
			d2dCtx->SetTarget(nullptr);
			m_d2dTargetBitmap = nullptr;
			m_d3dDepthStencilView = nullptr;
			d3dCtx->Flush1(D3D11_CONTEXT_TYPE_ALL, nullptr);

			UpdateRenderTargetSize();

			// The width and height of the swap chain must be based on the window's
			// natively-oriented width and height. If the window is not in the native
			// orientation, the dimensions must be reversed.
			DXGI_MODE_ROTATION displayRotation = ComputeDisplayRotation();

			bool swapDimensions = displayRotation == DXGI_MODE_ROTATION_ROTATE90 || displayRotation == DXGI_MODE_ROTATION_ROTATE270;
			m_d3dRenderTargetSize.width = swapDimensions ? m_outputSize.height : m_outputSize.width;
			m_d3dRenderTargetSize.height = swapDimensions ? m_outputSize.width : m_outputSize.height;

			if (this->dxgiSwapChain != nullptr) {
				// If the swap chain already exists, resize it.
				HRESULT hr = this->dxgiSwapChain->ResizeBuffers(
					2, // Double-buffered swap chain.
					lround(m_d3dRenderTargetSize.width),
					lround(m_d3dRenderTargetSize.height),
					DXGI_FORMAT_B8G8R8A8_UNORM,
					0
				);

				if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
					// If the device was removed for any reason, a new device and swap chain will need to be created.
					HandleDeviceLost();

					// Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method 
					// and correctly set up the new device.
					return;
				}
				else {
					H::System::ThrowIfFailed(hr);
				}
			}
			else {
				// Otherwise, create a new one using the same adapter as the existing Direct3D device.
				//DXGI_SCALING scaling = DisplayMetrics::SupportHighResolutions ? DXGI_SCALING_NONE : DXGI_SCALING_STRETCH;
				DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };

				swapChainDesc.Width = lround(m_d3dRenderTargetSize.width);		// Match the size of the window.
				swapChainDesc.Height = lround(m_d3dRenderTargetSize.height);
				swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;				// This is the most common swap chain format.
				swapChainDesc.Stereo = false;
				swapChainDesc.SampleDesc.Count = 1;								// Don't use multi-sampling.
				swapChainDesc.SampleDesc.Quality = 0;
				swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				swapChainDesc.BufferCount = 2;									// Use double-buffering to minimize latency.
				swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;	// All Microsoft Store apps must use _FLIP_ SwapEffects.
				swapChainDesc.Flags = 0;
				swapChainDesc.Scaling = DXGI_SCALING_STRETCH;					// CreateSwapChainForComposition support only DXGI_SCALING_STRETCH
				swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

				// This sequence obtains the DXGI factory that was used to create the Direct3D device above.
				Microsoft::WRL::ComPtr<IDXGIDevice3> dxgiDevice;
				hr = d3dDevice.As(&dxgiDevice);
				H::System::ThrowIfFailed(hr);

				Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
				hr = dxgiDevice->GetAdapter(&dxgiAdapter);
				H::System::ThrowIfFailed(hr);

				Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
				hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
				H::System::ThrowIfFailed(hr);

				Microsoft::WRL::ComPtr<IDXGISwapChain1> dxgiSwapChain1;

				switch (this->environment) {
				case Environment::Desktop: {
					hr = dxgiFactory->CreateSwapChainForHwnd(
						d3dDevice.Get(),
						this->hWnd,
						&swapChainDesc,
						nullptr,
						nullptr,
						&dxgiSwapChain1
					);
					H::System::ThrowIfFailed(hr);

					hr = dxgiSwapChain1.As(&this->dxgiSwapChain);
					H::System::ThrowIfFailed(hr);
					break;
				}
				
				case Environment::UWP: {
					// When using XAML interop, the swap chain must be created for composition.
					hr = dxgiFactory->CreateSwapChainForComposition(
						d3dDevice.Get(),
						&swapChainDesc,
						nullptr,
						&dxgiSwapChain1
					);
					H::System::ThrowIfFailed(hr);

					hr = dxgiSwapChain1.As(&this->dxgiSwapChain);
					H::System::ThrowIfFailed(hr);

					if (!this->swapChainCreateFn) {
						throw std::exception{ "Cannot create swap chain for UWP environment bacause this->swapChainCreateFn is empty." };
					}
					this->swapChainCreateFn(this->dxgiSwapChain.Get());
					break;
				}
				}

				// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
				// ensures that the application will only render after each VSync, minimizing power consumption.
				hr = dxgiDevice->SetMaximumFrameLatency(1);
				H::System::ThrowIfFailed(hr);
			}

			// Set the proper orientation for the swap chain, and generate 2D and
			// 3D matrix transformations for rendering to the rotated swap chain.
			// Note the rotation angle for the 2D and 3D transforms are different.
			// This is due to the difference in coordinate spaces.  Additionally,
			// the 3D matrix is specified explicitly to avoid rounding errors.

			switch (displayRotation) {
			case DXGI_MODE_ROTATION_IDENTITY:
				m_orientationTransform2D = D2D1::Matrix3x2F::Identity();
				m_orientationTransform3D = ScreenRotation::Rotation0;
				break;

			case DXGI_MODE_ROTATION_ROTATE90:
				m_orientationTransform2D =
					D2D1::Matrix3x2F::Rotation(90.0f) *
					D2D1::Matrix3x2F::Translation(m_logicalSize.height, 0.0f);
				m_orientationTransform3D = ScreenRotation::Rotation270;
				break;

			case DXGI_MODE_ROTATION_ROTATE180:
				m_orientationTransform2D =
					D2D1::Matrix3x2F::Rotation(180.0f) *
					D2D1::Matrix3x2F::Translation(m_logicalSize.width, m_logicalSize.height);
				m_orientationTransform3D = ScreenRotation::Rotation180;
				break;

			case DXGI_MODE_ROTATION_ROTATE270:
				m_orientationTransform2D =
					D2D1::Matrix3x2F::Rotation(270.0f) *
					D2D1::Matrix3x2F::Translation(0.0f, m_logicalSize.width);
				m_orientationTransform3D = ScreenRotation::Rotation90;
				break;

			default:
				throw std::exception{ "'displayRotation' unspecified" };
			}

			hr = this->dxgiSwapChain->SetRotation(displayRotation);
			H::System::ThrowIfFailed(hr);

			if (this->environment == Environment::UWP) {
				// Setup inverse scale on the swap chain
				DXGI_MATRIX_3X2_F inverseScale = { 0 };
				inverseScale._11 = 1.0f / m_effectiveCompositionScaleX;
				inverseScale._22 = 1.0f / m_effectiveCompositionScaleY;
				Microsoft::WRL::ComPtr<IDXGISwapChain2> swapChain2;
				hr = this->dxgiSwapChain.As<IDXGISwapChain2>(&swapChain2);
				H::System::ThrowIfFailed(hr);

				hr = swapChain2->SetMatrixTransform(&inverseScale);
				H::System::ThrowIfFailed(hr);
			}

			// Create a render target view of the swap chain back buffer.
			Microsoft::WRL::ComPtr<ID3D11Texture2D1> backBuffer;
			hr = this->dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
			H::System::ThrowIfFailed(hr);

			hr = d3dDevice->CreateRenderTargetView1(
				backBuffer.Get(),
				nullptr,
				&m_d3dRenderTargetView
			);
			H::System::ThrowIfFailed(hr);

			// Create a depth stencil view for use with 3D rendering if needed.
			CD3D11_TEXTURE2D_DESC1 depthStencilDesc(
				DXGI_FORMAT_D24_UNORM_S8_UINT,
				lround(m_d3dRenderTargetSize.width),
				lround(m_d3dRenderTargetSize.height),
				1, // This depth stencil view has only one texture.
				1, // Use a single mipmap level.
				D3D11_BIND_DEPTH_STENCIL
			);

			Microsoft::WRL::ComPtr<ID3D11Texture2D1> depthStencil;
			hr = d3dDevice->CreateTexture2D1(
				&depthStencilDesc,
				nullptr,
				&depthStencil
			);
			H::System::ThrowIfFailed(hr);

			CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
			hr = d3dDevice->CreateDepthStencilView(
				depthStencil.Get(),
				&depthStencilViewDesc,
				&m_d3dDepthStencilView
			);
			H::System::ThrowIfFailed(hr);

			// Set the 3D rendering viewport to target the entire window.
			m_screenViewport = CD3D11_VIEWPORT(
				0.0f,
				0.0f,
				m_d3dRenderTargetSize.width,
				m_d3dRenderTargetSize.height
			);

			d3dCtx->RSSetViewports(1, &m_screenViewport);

			// Create a Direct2D target bitmap associated with the
			// swap chain back buffer and set it as the current target.
			D2D1_BITMAP_PROPERTIES1 bitmapProperties =
				D2D1::BitmapProperties1(
					D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
					D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
					m_dpi,
					m_dpi
				);

			Microsoft::WRL::ComPtr<IDXGISurface2> dxgiBackBuffer;
			hr = this->dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));
			H::System::ThrowIfFailed(hr);

			hr = d2dCtx->CreateBitmapFromDxgiSurface(
				dxgiBackBuffer.Get(),
				&bitmapProperties,
				&m_d2dTargetBitmap
			);
			H::System::ThrowIfFailed(hr);

			d2dCtx->SetTarget(m_d2dTargetBitmap.Get());
			d2dCtx->SetDpi(m_effectiveDpi, m_effectiveDpi);

			// Grayscale text anti-aliasing is recommended for all Microsoft Store apps.
			d2dCtx->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
		}

		// Determine the dimensions of the render target and whether it will be scaled down.
		void SwapChainPanel::UpdateRenderTargetSize() {
			m_effectiveDpi = m_dpi;
			m_effectiveCompositionScaleX = m_compositionScaleX;
			m_effectiveCompositionScaleY = m_compositionScaleY;

			// To improve battery life on high resolution devices, render to a smaller render target
			// and allow the GPU to scale the output when it is presented.
			if (!DisplayMetrics::SupportHighResolutions && m_dpi > DisplayMetrics::DpiThreshold)
			{
				float width = H::Dx::ConvertDipsToPixels(m_logicalSize.width, m_dpi);
				float height = H::Dx::ConvertDipsToPixels(m_logicalSize.height, m_dpi);

				// When the device is in portrait orientation, height > width. Compare the
				// larger dimension against the width threshold and the smaller dimension
				// against the height threshold.
				//if (std::max(width, height) > DisplayMetrics::WidthThreshold && std::min(width, height) > DisplayMetrics::HeightThreshold)
				//{
					// To scale the app we change the effective DPI. Logical size does not change.
					m_effectiveDpi /= this->m_resolutionScale;
					m_effectiveCompositionScaleX /= this->m_resolutionScale;
					m_effectiveCompositionScaleY /= this->m_resolutionScale;
				//}
			}

			// Calculate the necessary render target size in pixels.
			m_outputSize.width = H::Dx::ConvertDipsToPixels(m_logicalSize.width, m_effectiveDpi);
			m_outputSize.height = H::Dx::ConvertDipsToPixels(m_logicalSize.height, m_effectiveDpi);

			// Prevent zero size DirectX content from being created.
			m_outputSize.width = std::max(m_outputSize.width, 1.0f);
			m_outputSize.height = std::max(m_outputSize.height, 1.0f);
		}

		// This method is called in the event handler for the SizeChanged event.
		void SwapChainPanel::SetLogicalSize(H::Size_f logicalSize) {
			if (m_logicalSize != logicalSize) {
				m_logicalSize = logicalSize;
				CreateWindowSizeDependentResources();
			}
		}

		// This method is called in the event handler for the DpiChanged event.
		void SwapChainPanel::SetDpi(float dpi) {
			auto dxDev = this->dxDeviceSafeObj.Lock();
			auto dxCtx = dxDev->LockContext();
			auto d2dCtx = dxCtx->D2D();

			if (dpi != m_dpi) {
				m_dpi = dpi;
				d2dCtx->SetDpi(m_dpi, m_dpi);
				CreateWindowSizeDependentResources();
			}
		}

		void SwapChainPanel::SetNativeOrientation(DisplayOrientations nativeOrientation) {
			this->m_nativeOrientation = nativeOrientation;
		}

		// This method is called in the event handler for the OrientationChanged event.
		void SwapChainPanel::SetCurrentOrientation(DisplayOrientations currentOrientation) {
			if (m_currentOrientation != currentOrientation) {
				m_currentOrientation = currentOrientation;
				CreateWindowSizeDependentResources();
			}
		}

		// This method is called in the event handler for the CompositionScaleChanged event.
		void SwapChainPanel::SetCompositionScale(float compositionScaleX, float compositionScaleY) {
			if (m_compositionScaleX != compositionScaleX ||
				m_compositionScaleY != compositionScaleY)
			{
				m_compositionScaleX = compositionScaleX;
				m_compositionScaleY = compositionScaleY;
				CreateWindowSizeDependentResources();
			}
		}

		void SwapChainPanel::SetRenderResolutionScale(float resolutionScale) {
			if (m_resolutionScale != resolutionScale) {
				m_resolutionScale = resolutionScale;
				CreateWindowSizeDependentResources();
			}
		}

		// This method is called in the event handler for the DisplayContentsInvalidated event.
		void SwapChainPanel::ValidateDevice() {
			HRESULT hr = S_OK;
			auto dxDev = this->dxDeviceSafeObj.Lock();
			auto d3dDevice = dxDev->GetD3DDevice();

			// The D3D Device is no longer valid if the default adapter changed since the device
			// was created or if the device has been removed.

			// First, get the information for the default adapter from when the device was created.

			Microsoft::WRL::ComPtr<IDXGIDevice3> dxgiDevice;
			hr = d3dDevice.As(&dxgiDevice);
			H::System::ThrowIfFailed(hr);

			Microsoft::WRL::ComPtr<IDXGIAdapter> deviceAdapter;
			hr = dxgiDevice->GetAdapter(&deviceAdapter);
			H::System::ThrowIfFailed(hr);

			Microsoft::WRL::ComPtr<IDXGIFactory2> deviceFactory;
			hr = deviceAdapter->GetParent(IID_PPV_ARGS(&deviceFactory));
			H::System::ThrowIfFailed(hr);

			Microsoft::WRL::ComPtr<IDXGIAdapter1> previousDefaultAdapter;
			hr = deviceFactory->EnumAdapters1(0, &previousDefaultAdapter);
			H::System::ThrowIfFailed(hr);

			DXGI_ADAPTER_DESC1 previousDesc;
			hr = previousDefaultAdapter->GetDesc1(&previousDesc);
			H::System::ThrowIfFailed(hr);

			// Next, get the information for the current default adapter.

			Microsoft::WRL::ComPtr<IDXGIFactory4> currentFactory;
			hr = CreateDXGIFactory1(IID_PPV_ARGS(&currentFactory));
			H::System::ThrowIfFailed(hr);

			Microsoft::WRL::ComPtr<IDXGIAdapter1> currentDefaultAdapter;
			hr = currentFactory->EnumAdapters1(0, &currentDefaultAdapter);
			H::System::ThrowIfFailed(hr);

			DXGI_ADAPTER_DESC1 currentDesc;
			hr = currentDefaultAdapter->GetDesc1(&currentDesc);
			H::System::ThrowIfFailed(hr);

			// If the adapter LUIDs don't match, or if the device reports that it has been removed,
			// a new D3D device must be created.

			if (previousDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart ||
				previousDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart ||
				FAILED(d3dDevice->GetDeviceRemovedReason()))
			{
				// Release references to resources related to the old device.
				dxgiDevice = nullptr;
				deviceAdapter = nullptr;
				deviceFactory = nullptr;
				previousDefaultAdapter = nullptr;

				// Create a new device and swap chain.
				HandleDeviceLost();
			}
		}

		// Recreate all device resources and set them back to the current state.
		void SwapChainPanel::HandleDeviceLost() {
			assert(false);
			// TODO: implement
			//auto dxDev = this->dxDeviceSafeObj.Lock();

			//this->dxgiSwapChain = nullptr;

			//if (m_deviceNotify != nullptr) {
			//	m_deviceNotify->OnDeviceLost();
			//}

			//CreateDeviceResources();
			//m_d2dContext->SetDpi(m_dpi, m_dpi);
			//CreateWindowSizeDependentResources();

			//if (m_deviceNotify != nullptr) {
			//	m_deviceNotify->OnDeviceRestored();
			//}
		}

		// Register our DeviceNotify to be informed on device lost and creation.
		void SwapChainPanel::RegisterDeviceNotify(IDeviceNotify* deviceNotify) {
			m_deviceNotify = deviceNotify;
		}

		// Call this method when the app suspends. It provides a hint to the driver that the app 
		// is entering an idle state and that temporary buffers can be reclaimed for use by other apps.
		void SwapChainPanel::Trim() {
			auto dxDev = this->dxDeviceSafeObj.Lock();
			auto d3dDevice = dxDev->GetD3DDevice();

			Microsoft::WRL::ComPtr<IDXGIDevice3> dxgiDevice;
			d3dDevice.As(&dxgiDevice);

			dxgiDevice->Trim();
		}

		// Present the contents of the swap chain to the screen.
		void SwapChainPanel::Present() {
			auto dxDev = this->dxDeviceSafeObj.Lock();
			auto dxCtx = dxDev->LockContext();
			auto d3dCtx = dxCtx->D3D();

			// The first argument instructs DXGI to block until VSync, putting the application
			// to sleep until the next VSync. This ensures we don't waste any cycles rendering
			// frames that will never be displayed to the screen.
			DXGI_PRESENT_PARAMETERS parameters = { 0 };
			HRESULT hr = this->dxgiSwapChain->Present1(1, 0, &parameters);

			// Discard the contents of the render target.
			// This is a valid operation only when the existing contents will be entirely
			// overwritten. If dirty or scroll rects are used, this call should be modified.
			d3dCtx->DiscardView1(m_d3dRenderTargetView.Get(), nullptr, 0);

			// Discard the contents of the depth stencil.
			d3dCtx->DiscardView1(m_d3dDepthStencilView.Get(), nullptr, 0);

			// If the device was removed either by a disconnection or a driver upgrade, we 
			// must recreate all device resources.
			if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
				HandleDeviceLost();
			}
			else {
				H::System::ThrowIfFailed(hr);
			}
		}


		H::Size_f SwapChainPanel::GetOutputSize() const {
			return m_outputSize;
		}

		H::Size_f SwapChainPanel::GetLogicalSize() const {
			return m_logicalSize;
		}

		H::Size_f SwapChainPanel::GetRenderTargetSize() const {
			return m_d3dRenderTargetSize;
		}

		float SwapChainPanel::GetDpi() const {
			return m_effectiveDpi;
		}

		IDXGISwapChain3* SwapChainPanel::GetSwapChain() const {
			return this->dxgiSwapChain.Get();
		}

		ID3D11RenderTargetView1* SwapChainPanel::GetBackBufferRenderTargetView() const {
			return m_d3dRenderTargetView.Get();
		}

		ID3D11DepthStencilView* SwapChainPanel::GetDepthStencilView() const {
			return m_d3dDepthStencilView.Get();
		}

		D3D11_VIEWPORT SwapChainPanel::GetScreenViewport() const {
			return m_screenViewport;
		}

		DirectX::XMFLOAT4X4 SwapChainPanel::GetOrientationTransform3D() const {
			return m_orientationTransform3D;
		}

		ID2D1Bitmap1* SwapChainPanel::GetD2DTargetBitmap() const {
			return m_d2dTargetBitmap.Get();
		}

		D2D1::Matrix3x2F SwapChainPanel::GetOrientationTransform2D() const {
			return m_orientationTransform2D;
		}



		// This method determines the rotation between the display device's native orientation and the
		// current display orientation.
		DXGI_MODE_ROTATION SwapChainPanel::ComputeDisplayRotation() {
			DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;

			// Note: NativeOrientation can only be Landscape or Portrait even though
			// the DisplayOrientations enum has other values.
			switch (m_nativeOrientation) {
			case DisplayOrientations::Landscape:
				switch (m_currentOrientation)
				{
				case DisplayOrientations::Landscape:
					rotation = DXGI_MODE_ROTATION_IDENTITY;
					break;

				case DisplayOrientations::Portrait:
					rotation = DXGI_MODE_ROTATION_ROTATE270;
					break;

				case DisplayOrientations::LandscapeFlipped:
					rotation = DXGI_MODE_ROTATION_ROTATE180;
					break;

				case DisplayOrientations::PortraitFlipped:
					rotation = DXGI_MODE_ROTATION_ROTATE90;
					break;
				}
				break;

			case DisplayOrientations::Portrait:
				switch (m_currentOrientation)
				{
				case DisplayOrientations::Landscape:
					rotation = DXGI_MODE_ROTATION_ROTATE90;
					break;

				case DisplayOrientations::Portrait:
					rotation = DXGI_MODE_ROTATION_IDENTITY;
					break;

				case DisplayOrientations::LandscapeFlipped:
					rotation = DXGI_MODE_ROTATION_ROTATE270;
					break;

				case DisplayOrientations::PortraitFlipped:
					rotation = DXGI_MODE_ROTATION_ROTATE180;
					break;
				}
				break;
			}
			return rotation;
		}
	}
}