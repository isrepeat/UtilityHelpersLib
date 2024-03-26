#pragma once
#include "common.h"
#include "Scope.h"
#include <windows.h>
#include <string>
#include <functional>

namespace HELPERS_NS {
	class Semaphore {
	public:
		Semaphore(const std::wstring& name);
		Semaphore(const Semaphore&) = delete;
		Semaphore(Semaphore&&);
		~Semaphore();

		void Lock();
		void Unlock();

		HELPERS_NS::Scope<std::function<void()>> LockScoped();

	private:
		HANDLE semaphore{};
	};
}