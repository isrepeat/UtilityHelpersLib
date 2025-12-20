#pragma once
#include <Helpers/common.h>
#include <Helpers/UniqueHandle.h>
#include <condition_variable>
#include <d3d11_1.h>
#include <d3d11.h>
#include <cstdint>
#include <vector>
#include <memory>
#include <mutex>
#include <wrl.h>

namespace HELPERS_NS {
    namespace Dx {
        class DxSharedTextureLocked;
        class DxSharedTextureLocker;

        // creates texture shared between 2 ID3D11Device's
        class DxSharedTexture {
        public:
            DxSharedTexture() = default;
            DxSharedTexture(
                const D3D11_TEXTURE2D_DESC& desc,
                Microsoft::WRL::ComPtr<ID3D11Device> dstDevice,
                Microsoft::WRL::ComPtr<ID3D11Device> srcDevice,
                uint32_t poolSize = 3
            );

            explicit operator bool() const;

            // Legacy: returns locked texture for slot 0 (kept for compatibility)
            DxSharedTextureLocked GetLockedTextureOnDstDevice() const;
            DxSharedTextureLocked GetLockedTextureOnSrcDevice() const;

            // Legacy slow path: 2 copies + CreateTexture2D each call (kept for compatibility)
            void CopyTexture(
                ID3D11Texture2D** ppDstTexture,
                const Microsoft::WRL::ComPtr<ID3D11Texture2D>& srcTexture
            );

            // New fast path:
            // 1) Copy srcTexture -> shared slot texture (on srcDevice)
            // 2) Return shared texture opened on dstDevice (NO CreateTexture2D, NO second copy)
            //
            // outRenderLease holds a keyed mutex lock (consumer) and keeps slot in-use until released.
            void CopyTextureShared(
                ID3D11Texture2D** ppDstTexture,
                std::shared_ptr<void>* outRenderLease,
                const Microsoft::WRL::ComPtr<ID3D11Texture2D>& srcTexture
            );

            uint32_t AcquireFreeSlotIndex();
            void ReleaseSlotIndex(uint32_t slotIndex);

        private:
            class RenderLease;

            struct Slot {
                Microsoft::WRL::ComPtr<ID3D11Texture2D> dstDeviceTexture;
                Microsoft::WRL::ComPtr<IDXGIKeyedMutex> dstDeviceTextureMtx;

                UHANDLE sharedTextureHandle;

                Microsoft::WRL::ComPtr<ID3D11Texture2D> srcDeviceTexture;
                Microsoft::WRL::ComPtr<IDXGIKeyedMutex> srcDeviceTextureMtx;

                bool inUse = false;
            };

            Microsoft::WRL::ComPtr<ID3D11Device> dstDevice;
            Microsoft::WRL::ComPtr<ID3D11Device> srcDevice;

            std::vector<Slot> slots;

            mutable std::mutex slotsMutex;
            mutable std::condition_variable slotsCv;
        };


        class DxSharedTextureLocked {
        public:
            DxSharedTextureLocked(
                Microsoft::WRL::ComPtr<ID3D11Texture2D> tex,
                Microsoft::WRL::ComPtr<IDXGIKeyedMutex> texMtx
            );
            ~DxSharedTextureLocked();

            ID3D11Texture2D* GetTexture() const;

        private:
            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
            Microsoft::WRL::ComPtr<IDXGIKeyedMutex> texMtx;
        };


        // allows use texture only when texture mutex is locked
        class DxSharedTextureLocker {
        public:
            DxSharedTextureLocker() = default;
            DxSharedTextureLocker(
                const Microsoft::WRL::ComPtr<ID3D11Texture2D>& tex,
                const Microsoft::WRL::ComPtr<IDXGIKeyedMutex>& texMtx
            );

            ID3D11Texture2D* GetTexture() const;

            // https://en.cppreference.com/w/cpp/named_req/BasicLockable
            void lock();
            void unlock();

        private:
            bool locked = false;
            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
            Microsoft::WRL::ComPtr<IDXGIKeyedMutex> texMtx;
        };
    }
}