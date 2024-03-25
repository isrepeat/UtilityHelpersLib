#include "EventObject.h"
#include "System.h"

namespace HELPERS_NS {
	EventObject::EventObject(const std::wstring& name) {
		auto fullName = L"Global\\" + name;
		event = CreateEvent(nullptr, TRUE, TRUE, fullName.c_str());
		auto lastError = GetLastError();
		if (event == INVALID_HANDLE_VALUE) {
			HELPERS_NS::System::ThrowIfFailed(E_FAIL);
		}
	}

	EventObject::EventObject(EventObject&& other)
		: event(other.event)
	{
		other.event = nullptr;
	}

	EventObject::~EventObject() {
		CloseHandle(event);
	}

	void EventObject::Wait(DWORD timeoutMs) {
		DWORD result = WaitForSingleObject(event, timeoutMs);
		if (result == WAIT_FAILED) {
			HELPERS_NS::System::ThrowIfFailed(E_FAIL);
		}
	}

	void EventObject::Signal() {
		BOOL result = SetEvent(event);
		if (!result) {
			HELPERS_NS::System::ThrowIfFailed(E_FAIL);
		}
	}

	void EventObject::Reset() {
		BOOL result = ResetEvent(event);
		if (!result) {
			HELPERS_NS::System::ThrowIfFailed(E_FAIL);
		}
	}

	HELPERS_NS::Scope<std::function<void()>> EventObject::ResetScoped() {
		Reset();
		return { [this] { Signal(); } };
	}
}