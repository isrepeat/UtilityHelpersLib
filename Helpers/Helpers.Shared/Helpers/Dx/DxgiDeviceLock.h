#pragma once
#include <Helpers/common.h>
#include <Helpers/UniqueHandle.h>
#include <Helpers/Memory.h>
#include <Helpers/Macros.h>
#include <mfobjects.h>
#include <d3d11_1.h>
#include <d3d11.h>
#include <wrl.h>

namespace HELPERS_NS {
	namespace Dx {
		template<class DeviceManagerT>
		class DxgiDeviceLockBase {
		protected:
			DxgiDeviceLockBase(Microsoft::WRL::ComPtr<DeviceManagerT> deviceManager)
				: deviceManager{ deviceManager }
				, hDevice{ nullptr }
				, locked{ false }
			{
				deviceManager->OpenDeviceHandle(&hDevice);
			}

			~DxgiDeviceLockBase() {
				if (hDevice) {
					if (locked) {
						deviceManager->UnlockDevice(hDevice, FALSE);
					}
					deviceManager->CloseDeviceHandle(hDevice);
				}
			}

			NO_COPY(DxgiDeviceLockBase);

		protected:
			Microsoft::WRL::ComPtr<DeviceManagerT> deviceManager;
			HANDLE hDevice;
			bool locked;
		};


		class MFDXGIDeviceManagerLock : public DxgiDeviceLockBase<IMFDXGIDeviceManager> {
		public:
			MFDXGIDeviceManagerLock(_In_ Microsoft::WRL::ComPtr<IMFDXGIDeviceManager> deviceManager)
				: DxgiDeviceLockBase(deviceManager)
			{}

			template <typename DeviceInterfaceT>
			HRESULT LockDevice(_Out_ DeviceInterfaceT** deviceInterface) {
				HRESULT hr = S_OK;

				if (locked) {
					hr = E_FAIL;
				}

				hr = deviceManager->LockDevice(hDevice, __uuidof(DeviceInterfaceT), (void**)deviceInterface, TRUE);
				if (SUCCEEDED(hr)) {
					locked = true;
				}
				return hr;
			}
		};
	}
}