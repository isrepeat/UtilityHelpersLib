#pragma once
#include "libhelpers/config.h"
#include "IOutput.h"
#include "../Dx.h"
#include "../../raw_ptr.h"

#include "libhelpers/Aligned.h"

#if HAVE_WINRT == 1

class SwapChainPanelOutput : public IOutput {
	static const uint32_t BufferCount = 2;
	static const DXGI_FORMAT BufferFmt = DXGI_FORMAT_B8G8R8A8_UNORM;
public:
	SwapChainPanelOutput(
		raw_ptr<DxDevice> dxDev,
		Windows::UI::Xaml::Controls::SwapChainPanel^ swapChainPanel);
	virtual ~SwapChainPanelOutput();

	float GetLogicalDpi() const override;
	DirectX::XMFLOAT2 GetLogicalSize() const override;
	D3D11_VIEWPORT GetD3DViewport() const override;
	ID3D11RenderTargetView *GetD3DRtView() const override;
	ID2D1Bitmap1 *GetD2DRtView() const override;
	D2D1_MATRIX_3X2_F GetD2DOrientationTransform() const override;
	DirectX::XMFLOAT4X4 GetD3DOrientationTransform() const override;
	OrientationTypes GetOrientation() const override;
	OrientationTypes GetNativeOrientation() const override;
	
	DirectX::XMFLOAT4 GetRTColor() const;
	void SetRTColor(DirectX::XMFLOAT4 color);

	Helpers_WinRt::Dx::DxSettings^ GetDxSettings() const;
	Windows::UI::Xaml::Controls::SwapChainPanel^ GetSwapChainPanel() const;

	void SetLogicalDpi(float v);
	void SetLogicalSize(const DirectX::XMFLOAT2 &v);
	DirectX::XMFLOAT2 GetCompositionScale() const;
	void SetCompositionScale(const DirectX::XMFLOAT2 &v);
	Windows::Graphics::Display::DisplayOrientations GetCurrentOrientation() const;
	void SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations v);
	void Resize();

	void Render(std::function<void()> renderHandler);

protected:
	void BeginRender();
	void EndRender();
	void Present();

private:
	void CreateWindowSizeDependentResources();
	void CreateSwapChain();

	void UpdatePresentationParameters();

	DXGI_MODE_ROTATION ComputeDisplayRotation();
	void SetRotationMatrices(DXGI_MODE_ROTATION rotation);

	// Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
	static float ConvertDipsToPixels(float dips, float dpi);

private:
	std::mutex mx;
	raw_ptr<DxDevice> dxDev;
	Helpers_WinRt::Dx::DxSettings^ dxSettings;
	Windows::Foundation::EventRegistrationToken dxSettingsMsaaChangedToken;
	Windows::Foundation::EventRegistrationToken dxSettingsCurrentAdapterChangedToken;

	Windows::UI::Xaml::Controls::SwapChainPanel^ swapChainPanel;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> d3dRenderTargetView;
	Microsoft::WRL::ComPtr<ID2D1Bitmap1> d2dTargetBitmap;

	// MSAA resources
	Microsoft::WRL::ComPtr<ID3D11Texture2D>         m_msaaRenderTarget;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_msaaRenderTargetView;

	Windows::Graphics::Display::DisplayOrientations nativeOrientation;
	Windows::Graphics::Display::DisplayOrientations currentOrientation;
	D2D1_MATRIX_3X2_F d2dOrientationTransform;
	DirectX::XMFLOAT4X4 d3dOrientationTransform;

	float logicalDpi;
	DirectX::XMFLOAT2 logicalSize;
	DirectX::XMFLOAT2 physicalSize;
	DirectX::XMFLOAT2 compositionScale;

	DirectX::XMFLOAT4 rtColor;
};
#endif