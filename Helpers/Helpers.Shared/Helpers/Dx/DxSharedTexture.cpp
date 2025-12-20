#include "DxSharedTexture.h"
#include <Helpers/System.h>
#include <Helpers/Memory.h>

#include <dxgi1_2.h>

namespace HELPERS_NS {
    namespace Dx {
        // Producer/Consumer keys:
        // - producer (srcDevice) acquires key 0, releases key 1 when frame is written
        // - consumer (dstDevice/render) acquires key 1, releases key 0 when frame is done
        constexpr UINT64 kKeyProducer = 0;
        constexpr UINT64 kKeyConsumer = 1;

        class DxSharedTexture::RenderLease {
        public:
            RenderLease() = default;

            RenderLease(
                DxSharedTexture* owner,
                uint32_t slotIndex,
                Microsoft::WRL::ComPtr<IDXGIKeyedMutex> dstMtx)
                : owner(owner)
                , slotIndex(slotIndex)
                , dstMtx(dstMtx) {
                HRESULT hr = this->dstMtx->AcquireSync(kKeyConsumer, INFINITE);
                HELPERS_NS::System::ThrowIfFailed(hr);
            }

            ~RenderLease() {
                if (this->dstMtx) {
                    HRESULT hr = this->dstMtx->ReleaseSync(kKeyProducer);
                    HELPERS_NS::System::ThrowIfFailed(hr);
                }

                if (this->owner) {
                    this->owner->ReleaseSlotIndex(this->slotIndex);
                }
            }

            RenderLease(const RenderLease&) = delete;
            RenderLease& operator=(const RenderLease&) = delete;

        private:
            DxSharedTexture* owner = nullptr;
            uint32_t slotIndex = 0;
            Microsoft::WRL::ComPtr<IDXGIKeyedMutex> dstMtx;
        };
        

        DxSharedTexture::DxSharedTexture(
            const D3D11_TEXTURE2D_DESC& desc,
            Microsoft::WRL::ComPtr<ID3D11Device> dstDevice,
            Microsoft::WRL::ComPtr<ID3D11Device> srcDevice,
            uint32_t poolSize)
            : dstDevice{ dstDevice }
            , srcDevice{ srcDevice } {

            if (poolSize == 0) {
                poolSize = 1;
            }

            this->slots.resize(poolSize);

            for (uint32_t i = 0; i < poolSize; ++i) {
                HRESULT hr = S_OK;

                auto descCopy = desc;
                descCopy.MiscFlags |= D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;

                hr = this->dstDevice->CreateTexture2D(&descCopy, nullptr, &this->slots[i].dstDeviceTexture);
                HELPERS_NS::System::ThrowIfFailed(hr);

                Microsoft::WRL::ComPtr<IDXGIResource1> dxgiRes;
                hr = this->slots[i].dstDeviceTexture.As(&dxgiRes);
                HELPERS_NS::System::ThrowIfFailed(hr);

                hr = this->slots[i].dstDeviceTexture.As(&this->slots[i].dstDeviceTextureMtx);
                HELPERS_NS::System::ThrowIfFailed(hr);

                hr = dxgiRes->CreateSharedHandle(
                    nullptr,
                    DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE,
                    nullptr,
                    HELPERS_NS::GetAddressOf(this->slots[i].sharedTextureHandle)
                );
                HELPERS_NS::System::ThrowIfFailed(hr);

                Microsoft::WRL::ComPtr<ID3D11Device1> srcDevice1;
                hr = this->srcDevice.As(&srcDevice1);
                HELPERS_NS::System::ThrowIfFailed(hr);

                hr = srcDevice1->OpenSharedResource1(
                    this->slots[i].sharedTextureHandle.get(),
                    IID_PPV_ARGS(&this->slots[i].srcDeviceTexture)
                );
                HELPERS_NS::System::ThrowIfFailed(hr);

                hr = this->slots[i].srcDeviceTexture.As(&this->slots[i].srcDeviceTextureMtx);
                HELPERS_NS::System::ThrowIfFailed(hr);

                // Initial state: producer can write first
                // (consumer AcquireSync(kKeyConsumer) will wait until producer releases kKeyConsumer)
                hr = this->slots[i].dstDeviceTextureMtx->ReleaseSync(kKeyProducer);
                if (FAILED(hr)) {
                    // ignore if driver/runtime rejects initial release; state will still work in practice
                    // because first producer AcquireSync will succeed and then ReleaseSync will enable consumer.
                }
            }
        }

        DxSharedTexture::operator bool() const {
            return !this->slots.empty();
        }

        uint32_t DxSharedTexture::AcquireFreeSlotIndex() {
            for (uint32_t i = 0; i < this->slots.size(); ++i) {
                if (!this->slots[i].inUse) {
                    this->slots[i].inUse = true;
                    return i;
                }
            }

            return UINT32_MAX;
        }

        void DxSharedTexture::ReleaseSlotIndex(uint32_t slotIndex) {
            {
                std::lock_guard<std::mutex> lock(this->slotsMutex);
                if (slotIndex < this->slots.size()) {
                    this->slots[slotIndex].inUse = false;
                }
            }

            this->slotsCv.notify_one();
        }

        // Legacy: slot 0
        DxSharedTextureLocked DxSharedTexture::GetLockedTextureOnDstDevice() const {
            return DxSharedTextureLocked(this->slots[0].dstDeviceTexture, this->slots[0].dstDeviceTextureMtx);
        }
        DxSharedTextureLocked DxSharedTexture::GetLockedTextureOnSrcDevice() const {
            return DxSharedTextureLocked(this->slots[0].srcDeviceTexture, this->slots[0].srcDeviceTextureMtx);
        }

        // Legacy slow path (unchanged semantics)
        void DxSharedTexture::CopyTexture(
            ID3D11Texture2D** ppDstTexture,
            const Microsoft::WRL::ComPtr<ID3D11Texture2D>& srcTexture) {
            HRESULT hr = S_OK;

            // Use slot 0 as shared intermediate
            {
                auto textureOnSrcDeviceLocked = this->GetLockedTextureOnSrcDevice();

                Microsoft::WRL::ComPtr<ID3D11DeviceContext> srcDeviceContext;
                this->srcDevice->GetImmediateContext(srcDeviceContext.GetAddressOf());

                // Copy srcTexture that allocated on srcDevice to shared texture
                srcDeviceContext->CopyResource(textureOnSrcDeviceLocked.GetTexture(), srcTexture.Get());
            }

            {
                auto textureOnDstDeviceLocked = this->GetLockedTextureOnDstDevice();

                D3D11_TEXTURE2D_DESC srcTextureDesc = {};
                srcTexture->GetDesc(&srcTextureDesc);

                hr = this->dstDevice->CreateTexture2D(&srcTextureDesc, nullptr, ppDstTexture);
                HELPERS_NS::System::ThrowIfFailed(hr);

                Microsoft::WRL::ComPtr<ID3D11DeviceContext> dstDeviceContext;
                this->dstDevice->GetImmediateContext(dstDeviceContext.GetAddressOf());

                // Copy from shared texture to dstTexture
                dstDeviceContext->CopyResource(*ppDstTexture, textureOnDstDeviceLocked.GetTexture());
            }
        }


        // CopyTextureShared returns an ALREADY AddRef()'ed ID3D11Texture2D* opened on
        // the render-device and backed by an internal shared-slot.
        //
        // The returned texture MUST be used together with outRenderLease.
        // outRenderLease holds the keyed-mutex and the slot ownership and
        // prevents slot reuse while the frame is in use.
        //
        // Because the pointer is already AddRef()'ed, the caller must take ownership
        // via Attach() or manually Release(). GetAddressOf() is safe only if this
        // contract is preserved.
        void DxSharedTexture::CopyTextureShared(
            ID3D11Texture2D** ppDstTexture,
            std::shared_ptr<void>* outRenderLease,
            const Microsoft::WRL::ComPtr<ID3D11Texture2D>& srcTexture) {
            if (!ppDstTexture || !outRenderLease) {
                return;
            }

            *ppDstTexture = nullptr;
            *outRenderLease = nullptr;

            if (!srcTexture || !(*this)) {
                return;
            }

            uint32_t slotIndex = UINT32_MAX;

            {
                std::unique_lock<std::mutex> lock(this->slotsMutex);

                // Wait until at least one slot is free.
                // IMPORTANT: predicate must be side-effect free.
                this->slotsCv.wait(
                    lock,
                    [this]() {
                        for (const Slot& slot : this->slots) {
                            if (!slot.inUse) {
                                return true;
                            }
                        }
                        return false;
                    });

                // Now acquire a free slot.
                for (uint32_t i = 0; i < this->slots.size(); ++i) {
                    if (!this->slots[i].inUse) {
                        this->slots[i].inUse = true;
                        slotIndex = i;
                        break;
                    }
                }
            }

            if (slotIndex == UINT32_MAX) {
                return;
            }

            Slot& slot = this->slots[slotIndex];

            // Producer (srcDevice): write frame
            {
                HRESULT hr = slot.srcDeviceTextureMtx->AcquireSync(kKeyProducer, INFINITE);
                HELPERS_NS::System::ThrowIfFailed(hr);

                Microsoft::WRL::ComPtr<ID3D11DeviceContext> srcDeviceContext;
                this->srcDevice->GetImmediateContext(srcDeviceContext.GetAddressOf());

                srcDeviceContext->CopyResource(slot.srcDeviceTexture.Get(), srcTexture.Get());

                hr = slot.srcDeviceTextureMtx->ReleaseSync(kKeyConsumer);
                HELPERS_NS::System::ThrowIfFailed(hr);
            }

            // Return texture opened on dstDevice; hold consumer lock + slot lifetime in outRenderLease.
            *ppDstTexture = slot.dstDeviceTexture.Get();
            (*ppDstTexture)->AddRef();

            auto leaseImpl = std::make_shared<DxSharedTexture::RenderLease>(
                this,
                slotIndex,
                slot.dstDeviceTextureMtx
            );

            *outRenderLease = std::shared_ptr<void>(leaseImpl, leaseImpl.get());
        }

        DxSharedTextureLocked::DxSharedTextureLocked(
            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex,
            Microsoft::WRL::ComPtr<IDXGIKeyedMutex> texMtx)
            : tex(tex)
            , texMtx(texMtx) {
            HRESULT hr = this->texMtx->AcquireSync(0, INFINITE);
            H::System::ThrowIfFailed(hr);
        }

        DxSharedTextureLocked::~DxSharedTextureLocked() {
            HRESULT hr = this->texMtx->ReleaseSync(0);
            H::System::ThrowIfFailed(hr);
        }

        ID3D11Texture2D* DxSharedTextureLocked::GetTexture() const {
            return this->tex.Get();
        }

        DxSharedTextureLocker::DxSharedTextureLocker(
            const Microsoft::WRL::ComPtr<ID3D11Texture2D>& tex,
            const Microsoft::WRL::ComPtr<IDXGIKeyedMutex>& texMtx)
            : tex(tex)
            , texMtx(texMtx) {
        }

        ID3D11Texture2D* DxSharedTextureLocker::GetTexture() const {
            assert(this->locked);
            return this->tex.Get();
        }

        void DxSharedTextureLocker::lock() {
            HRESULT hr = S_OK;

            hr = this->texMtx->AcquireSync(0, INFINITE);
            H::System::ThrowIfFailed(hr);

            this->locked = true;
        }

        void DxSharedTextureLocker::unlock() {
            HRESULT hr = S_OK;

            hr = this->texMtx->ReleaseSync(0);
            H::System::ThrowIfFailed(hr);

            this->locked = false;
        }
    }
}
