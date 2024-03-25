#include "Semaphore.h"
#include "System.h"

namespace HELPERS_NS {
	Semaphore::Semaphore(const std::wstring& name) {
		auto fullName = L"Global\\" + name;
		semaphore = CreateSemaphore(nullptr, 1, 1, fullName.data());
		auto lastError = GetLastError();
		if (semaphore == INVALID_HANDLE_VALUE) {
			HELPERS_NS::System::ThrowIfFailed(E_FAIL);
		}
	}

	Semaphore::Semaphore(Semaphore&& other)
		: semaphore(other.semaphore)
	{
		other.semaphore = nullptr;
	}

	Semaphore::~Semaphore() {
		CloseHandle(semaphore);
	}

	void Semaphore::Lock() {
		auto result = WaitForSingleObject(semaphore, INFINITE);
		if (result != WAIT_OBJECT_0) {
			HELPERS_NS::System::ThrowIfFailed(E_FAIL);
		}
	}

	void Semaphore::Unlock() {
		BOOL result = ReleaseSemaphore(semaphore, 1, nullptr);
		if (!result) {
			HELPERS_NS::System::ThrowIfFailed(E_FAIL);
		}
	}

	HELPERS_NS::Scope<std::function<void()>> Semaphore::LockScoped() {
		Lock();
		return { [this] { Unlock(); } };
	}
}