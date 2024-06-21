#include "pch.h"
#include "SwapChainPanelOutput.h"
#include "../../HSystem.h"
#include "../../WinRT/RunOnUIThread.h"

#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <windows.ui.xaml.media.dxinterop.h>

#if HAVE_WINRT == 1
using namespace Windows::Graphics::Display;

// Constants used to calculate screen rotations.
namespace ScreenRotation
{
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

SwapChainPanelOutput::SwapChainPanelOutput(raw_ptr<DxDevice> dxDev, Windows::UI::Xaml::Controls::SwapChainPanel^ swapChainPanel)
	: dxDev(dxDev)
	, dxSettings{ ref new Helpers::WinRt::Dx::DxSettings() }
	, swapChainPanel(swapChainPanel)
	, physicalSize(1.0f, 1.0f)
	, d2dOrientationTransform(D2D1::IdentityMatrix())
	, d3dOrientationTransform(ScreenRotation::Rotation0)
{
	{
		auto tmp = DirectX::Colors::LightGreen;
		this->rtColor = { tmp.f[0], tmp.f[1], tmp.f[2], tmp.f[3] };
	}

	H::RunOnUIThread(this->swapChainPanel->Dispatcher,
		[&]
		{
			auto displayInformation = Windows::Graphics::Display::DisplayInformation::GetForCurrentView();

			this->nativeOrientation = displayInformation->NativeOrientation;
			this->currentOrientation = displayInformation->CurrentOrientation;

			this->logicalDpi = displayInformation->LogicalDpi;

			this->logicalSize = DirectX::XMFLOAT2(
				(float)this->swapChainPanel->ActualWidth,
				(float)this->swapChainPanel->ActualHeight);

			this->compositionScale = DirectX::XMFLOAT2(
				this->swapChainPanel->CompositionScaleX,
				this->swapChainPanel->CompositionScaleY);
		});

	this->CreateWindowSizeDependentResources();

	// TODO: add guards for destroyed 'this'
	this->dxSettingsMsaaChangedToken = this->dxSettings->MsaaChanged += ref new Helpers::WinRt::EventHandler([this] {
		this->CreateWindowSizeDependentResources();
	});
	this->dxSettingsCurrentAdapterChangedToken = this->dxSettings->CurrentAdapterChanged += ref new Helpers::WinRt::EventHandler([this] {
		// recreate dxDev ...
		});
}

SwapChainPanelOutput::~SwapChainPanelOutput() {
	this->dxSettings->MsaaChanged -= this->dxSettingsMsaaChangedToken;
	this->dxSettings->CurrentAdapterChanged -= this->dxSettingsCurrentAdapterChangedToken;
}

float SwapChainPanelOutput::GetLogicalDpi() const {
	return this->logicalDpi;
}

DirectX::XMFLOAT2 SwapChainPanelOutput::GetLogicalSize() const {
	return this->logicalSize;
}

D3D11_VIEWPORT SwapChainPanelOutput::GetD3DViewport() const {
	D3D11_VIEWPORT viewport;

	viewport.TopLeftX = viewport.TopLeftY = 0.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.Width = this->physicalSize.x;
	viewport.Height = this->physicalSize.y;
	
	return viewport;
}

ID3D11RenderTargetView *SwapChainPanelOutput::GetD3DRtView() const {
	return m_msaaRenderTargetView ? m_msaaRenderTargetView.Get() : d3dRenderTargetView.Get();
}

ID2D1Bitmap1 *SwapChainPanelOutput::GetD2DRtView() const {
	return this->d2dTargetBitmap.Get();
}

D2D1_MATRIX_3X2_F SwapChainPanelOutput::GetD2DOrientationTransform() const {
	return this->d2dOrientationTransform;
}

DirectX::XMFLOAT4X4 SwapChainPanelOutput::GetD3DOrientationTransform() const {
	return this->d3dOrientationTransform;
}

DirectX::XMFLOAT4 SwapChainPanelOutput::GetRTColor() const {
	return this->rtColor;
}

void SwapChainPanelOutput::SetRTColor(DirectX::XMFLOAT4 color) {
	this->rtColor = color;
}

OrientationTypes SwapChainPanelOutput::GetOrientation() const {
	switch (this->currentOrientation) {
	case Windows::Graphics::Display::DisplayOrientations::Landscape:
		return OrientationTypes::Landscape;
		break;
	case Windows::Graphics::Display::DisplayOrientations::LandscapeFlipped:
		return OrientationTypes::FlippedLandscape;
		break;
	case Windows::Graphics::Display::DisplayOrientations::Portrait:
		return OrientationTypes::Portrait;
		break;
	case Windows::Graphics::Display::DisplayOrientations::PortraitFlipped:
		return OrientationTypes::FlippedPortrait;
		break;
	default:
		return OrientationTypes::None;
		break;
	}
}

OrientationTypes SwapChainPanelOutput::GetNativeOrientation() const {
	switch (this->nativeOrientation) {
	case Windows::Graphics::Display::DisplayOrientations::Landscape:
		return OrientationTypes::Landscape;
		break;
	case Windows::Graphics::Display::DisplayOrientations::LandscapeFlipped:
		return OrientationTypes::FlippedLandscape;
		break;
	case Windows::Graphics::Display::DisplayOrientations::Portrait:
		return OrientationTypes::Portrait;
		break;
	case Windows::Graphics::Display::DisplayOrientations::PortraitFlipped:
		return OrientationTypes::FlippedPortrait;
		break;
	default:
		return OrientationTypes::None;
		break;
	}
}

Helpers::WinRt::Dx::DxSettings^ SwapChainPanelOutput::GetDxSettings() const {
	return this->dxSettings;
}

Windows::UI::Xaml::Controls::SwapChainPanel^ SwapChainPanelOutput::GetSwapChainPanel() const {
	return this->swapChainPanel;
}

void SwapChainPanelOutput::SetLogicalDpi(float v) {
	this->logicalDpi = v;
}

void SwapChainPanelOutput::SetLogicalSize(const DirectX::XMFLOAT2 &v) {
	this->logicalSize = v;
}

DirectX::XMFLOAT2 SwapChainPanelOutput::GetCompositionScale() const {
	return this->compositionScale;
}

void SwapChainPanelOutput::SetCompositionScale(const DirectX::XMFLOAT2 &v) {
	this->compositionScale = v;
}

Windows::Graphics::Display::DisplayOrientations SwapChainPanelOutput::GetCurrentOrientation() const {
	return this->currentOrientation;
}

void SwapChainPanelOutput::SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations v) {
	this->currentOrientation = v;
}

void SwapChainPanelOutput::Resize() {
	this->CreateWindowSizeDependentResources();
}

void SwapChainPanelOutput::Render(std::function<void()> renderHandler) {
	std::lock_guard lk{ mx };
	BeginRender();
	if (renderHandler) {
		renderHandler();
	}
	EndRender();
}

void SwapChainPanelOutput::BeginRender() {
	ID3D11RenderTargetView *const targets[1] = { this->GetD3DRtView() };
	auto ctx = this->dxDev->GetContext();

	ctx->D3D()->OMSetRenderTargets(1, targets, nullptr);
	DirectX::XMVECTORF32 tmp = { 
        this->rtColor.x * this->rtColor.w,
		this->rtColor.y * this->rtColor.w,
		this->rtColor.z * this->rtColor.w,
		this->rtColor.w };
	ctx->D3D()->ClearRenderTargetView(this->GetD3DRtView(), tmp);
}

void SwapChainPanelOutput::EndRender() {
	this->Present();
}

void SwapChainPanelOutput::Present() {
	if (m_msaaRenderTarget) {
		this->dxDev->GetContext()->D3D()->ResolveSubresource(backBuffer.Get(), 0, m_msaaRenderTarget.Get(), 0, SwapChainPanelOutput::BufferFmt);
	}

	// The first argument instructs DXGI to block until VSync, putting the application
	// to sleep until the next VSync. This ensures we don't waste any cycles rendering
	// frames that will never be displayed to the screen.
	HRESULT hr = this->swapChain->Present(dxSettings->VSync, 0);

	{
		auto ctx = this->dxDev->GetContext();

		// Discard the contents of the render target.
		// This is a valid operation only when the existing contents will be entirely
		// overwritten. If dirty or scroll rects are used, this call should be modified.
		ctx->D3D()->DiscardView1(this->d3dRenderTargetView.Get(), nullptr, 0);

		if (m_msaaRenderTargetView) {
			ctx->D3D()->DiscardView1(m_msaaRenderTargetView.Get(), nullptr, 0);
		}
	}

	//// If the device was removed either by a disconnection or a driver upgrade, we 
	//// must recreate all device resources.
	//if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	//{
	//	HandleDeviceLost();
	//}
	//else
	//{
	//	DX::ThrowIfFailed(hr);
	//}
}


void SwapChainPanelOutput::CreateWindowSizeDependentResources() {
	std::lock_guard lk{ mx };

	// Clear the previous window size specific context.
	HRESULT hr = S_OK;
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	auto d3dDev = this->dxDev->GetD3DDevice();
	auto d2dCtxMt = this->dxDev->GetD2DCtxMt();
	D2D1_BITMAP_PROPERTIES1 bitmapProperties =
		D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(SwapChainPanelOutput::BufferFmt, D2D1_ALPHA_MODE_PREMULTIPLIED),
			this->logicalDpi,
			this->logicalDpi
		);
	DXGI_MODE_ROTATION displayRotation = this->ComputeDisplayRotation();

	this->d3dRenderTargetView.Reset();
	this->d2dTargetBitmap.Reset();
	m_msaaRenderTarget.Reset();
	m_msaaRenderTargetView.Reset();
	backBuffer.Reset();

	{
		auto ctx = this->dxDev->GetContext();

		ctx->D2D()->SetTarget(nullptr);
		ctx->D3D()->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
		ctx->D3D()->Flush();
	}

	this->UpdatePresentationParameters();

	if (displayRotation == DXGI_MODE_ROTATION_ROTATE90 || displayRotation == DXGI_MODE_ROTATION_ROTATE270) {
		std::swap(this->physicalSize.x, this->physicalSize.y);
	}

	if (this->swapChain) {
		this->swapChain->ResizeBuffers(
			SwapChainPanelOutput::BufferCount,
			lround(this->physicalSize.x),
			lround(this->physicalSize.y),
			SwapChainPanelOutput::BufferFmt,
			0);
	}
	else {
		this->CreateSwapChain();
	}

	hr = this->swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	H::System::ThrowIfFailed(hr);

	hr = d3dDev->CreateRenderTargetView(
		backBuffer.Get(),
		nullptr,
		this->d3dRenderTargetView.GetAddressOf());
	H::System::ThrowIfFailed(hr);


	if (dxSettings->MSAA) {
		D3D11_TEXTURE2D_DESC desc = {};

		backBuffer->GetDesc(&desc);

		auto availableMsaaLevel = DxHelpers::MsaaHelper::GetMaxSupportedMSAA(d3dDev, desc.Format, DxHelpers::MsaaHelper::GetMaxMSAA());
		if (availableMsaaLevel) {
			desc.SampleDesc = *availableMsaaLevel;

			hr = d3dDev->CreateTexture2D(&desc, nullptr, &m_msaaRenderTarget);
			H::System::ThrowIfFailed(hr);

			hr = d3dDev->CreateRenderTargetView(m_msaaRenderTarget.Get(), nullptr, &m_msaaRenderTargetView);
			H::System::ThrowIfFailed(hr);
		}
	}

	Microsoft::WRL::ComPtr<IDXGISurface> dxgiBackBuffer;

	if (m_msaaRenderTarget) {
		hr = m_msaaRenderTarget.As(&dxgiBackBuffer);
	}
	else {
		hr = backBuffer.As(&dxgiBackBuffer);
	}
	H::System::ThrowIfFailed(hr);

	hr = d2dCtxMt->CreateBitmapFromDxgiSurface(
		dxgiBackBuffer.Get(),
		bitmapProperties,
		this->d2dTargetBitmap.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	this->SetRotationMatrices(displayRotation);

	Microsoft::WRL::ComPtr<IDXGISwapChain2> swapChain2;

	hr = this->swapChain.As(&swapChain2);
	H::System::ThrowIfFailed(hr);

	hr = swapChain2->SetRotation(displayRotation);
	H::System::ThrowIfFailed(hr);

	DXGI_MATRIX_3X2_F swapchainTransform = { 0 };

	swapchainTransform._11 = 1.0f / this->compositionScale.x;
	swapchainTransform._22 = 1.0f / this->compositionScale.y;

	hr = swapChain2->SetMatrixTransform(&swapchainTransform);
	H::System::ThrowIfFailed(hr);

	{
		auto viewport = this->GetD3DViewport();
		auto ctx = this->dxDev->GetContext();

		ctx->D3D()->RSSetViewports(1, &viewport);

		ctx->D2D()->SetTarget(this->d2dTargetBitmap.Get());
		ctx->D2D()->SetDpi(this->logicalDpi, this->logicalDpi);
		ctx->D2D()->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
	}
}

void SwapChainPanelOutput::CreateSwapChain() {
	// Otherwise, create a new one using the same adapter as the existing Direct3D device.
	HRESULT hr = S_OK;
	DXGI_SCALING scaling = DXGI_SCALING_STRETCH;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
	Microsoft::WRL::ComPtr<IDXGIDevice1> dxgiDevice;
	Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
	Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChainTmp;
	auto d3dDev = this->dxDev->GetD3DDeviceCPtr();

	swapChainDesc.Width = lround(this->physicalSize.x);		// Match the size of the window.
	swapChainDesc.Height = lround(this->physicalSize.y);
	swapChainDesc.Format = SwapChainPanelOutput::BufferFmt;				// This is the most common swap chain format.
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;								// Don't use multi-sampling.
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = SwapChainPanelOutput::BufferCount;									// Use double-buffering to minimize latency.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;	// All Windows Store apps must use _FLIP_ SwapEffects.
	swapChainDesc.Flags = 0;
	swapChainDesc.Scaling = scaling;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

	hr = d3dDev.As(&dxgiDevice);
	H::System::ThrowIfFailed(hr);

	hr = dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
	H::System::ThrowIfFailed(hr);

	hr = dxgiFactory->CreateSwapChainForComposition(d3dDev.Get(), &swapChainDesc, nullptr, swapChainTmp.GetAddressOf());
	H::System::ThrowIfFailed(hr);

	hr = swapChainTmp.As(&this->swapChain);
	H::System::ThrowIfFailed(hr);

	// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
	// ensures that the application will only render after each VSync, minimizing power consumption.
	hr = dxgiDevice->SetMaximumFrameLatency(1);
	H::System::ThrowIfFailed(hr);

	// Associate swap chain with SwapChainPanel
	// UI changes will need to be dispatched back to the UI thread
	H::RunOnUIThread(this->swapChainPanel->Dispatcher,
		[&]
		{
			// Get backing native interface for SwapChainPanel
			HRESULT hr = S_OK;
			Microsoft::WRL::ComPtr<ISwapChainPanelNative> panelNative;
			auto swapChainPanelUnk = reinterpret_cast<IUnknown*>(this->swapChainPanel);

			hr = swapChainPanelUnk->QueryInterface(IID_PPV_ARGS(panelNative.GetAddressOf()));
			H::System::ThrowIfFailed(hr);

			hr = panelNative->SetSwapChain(this->swapChain.Get());
			H::System::ThrowIfFailed(hr);
		});
}

void SwapChainPanelOutput::UpdatePresentationParameters() {
	this->logicalSize.x = (std::max)(this->logicalSize.x, 1.0f);
	this->logicalSize.y = (std::max)(this->logicalSize.y, 1.0f);

	this->physicalSize.x = this->ConvertDipsToPixels(this->logicalSize.x, this->logicalDpi);
	this->physicalSize.y = this->ConvertDipsToPixels(this->logicalSize.y, this->logicalDpi);
}

// This method determines the rotation between the display device's native orientation and the
// current display orientation.
DXGI_MODE_ROTATION SwapChainPanelOutput::ComputeDisplayRotation() {
	DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;

	// Note: NativeOrientation can only be Landscape or Portrait even though
	// the DisplayOrientations enum has other values.
	switch (this->nativeOrientation) {
	case DisplayOrientations::Landscape:
		switch (this->currentOrientation) {
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
		switch (this->currentOrientation) {
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

void SwapChainPanelOutput::SetRotationMatrices(DXGI_MODE_ROTATION rotation) {
	// Set the proper orientation for the swap chain, and generate 2D and
	// 3D matrix transformations for rendering to the rotated swap chain.
	// Note the rotation angle for the 2D and 3D transforms are different.
	// This is due to the difference in coordinate spaces.  Additionally,
	// the 3D matrix is specified explicitly to avoid rounding errors.
	switch (rotation) {
	case DXGI_MODE_ROTATION_IDENTITY:
		this->d2dOrientationTransform = D2D1::Matrix3x2F::Identity();
		this->d3dOrientationTransform = ScreenRotation::Rotation0;
		break;

	case DXGI_MODE_ROTATION_ROTATE90:
		this->d2dOrientationTransform =
			D2D1::Matrix3x2F::Rotation(90.0f) *
			D2D1::Matrix3x2F::Translation(this->logicalSize.y, 0.0f);
		this->d3dOrientationTransform = ScreenRotation::Rotation270;
		break;
	case DXGI_MODE_ROTATION_ROTATE180:
		this->d2dOrientationTransform =
			D2D1::Matrix3x2F::Rotation(180.0f) *
			D2D1::Matrix3x2F::Translation(this->logicalSize.x, this->logicalSize.y);
		this->d3dOrientationTransform = ScreenRotation::Rotation180;
		break;

	case DXGI_MODE_ROTATION_ROTATE270:
		this->d2dOrientationTransform =
			D2D1::Matrix3x2F::Rotation(270.0f) *
			D2D1::Matrix3x2F::Translation(0.0f, this->logicalSize.x);
		this->d3dOrientationTransform = ScreenRotation::Rotation90;
		break;

	default:
		throw ref new Platform::FailureException();
	}
}

float SwapChainPanelOutput::ConvertDipsToPixels(float dips, float dpi) {
	static const float dipsPerInch = 96.0f;
	return floorf(dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
}
#endif