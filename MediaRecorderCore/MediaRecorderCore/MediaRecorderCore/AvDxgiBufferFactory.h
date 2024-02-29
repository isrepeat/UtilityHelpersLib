#pragma once
#include "IAvDxBufferFactory.h"
#include "AvDxgiBufferFactoryFn.h"

class AvDxgiBufferFactory : public IAvDxBufferFactory {
public:
    AvDxgiBufferFactory(
        DxDevice *dxDev,
        MFCreateDXGISurfaceBufferPtr createSurfBuffer,
        MFCreateDXGIDeviceManagerPtr createDxMan);

    Microsoft::WRL::ComPtr<IMFMediaBuffer> CreateBuffer(ID3D11DeviceContext *d3dCtx, ID3D11Texture2D *tex) override;
    void SetAttributes(IMFAttributes *attr) override;

private:
    DxDevice *dxDev;
    MFCreateDXGISurfaceBufferPtr createSurfBuffer;
    MFCreateDXGIDeviceManagerPtr createDxMan;

    Microsoft::WRL::ComPtr<IMFDXGIDeviceManager> dxMan;
};
