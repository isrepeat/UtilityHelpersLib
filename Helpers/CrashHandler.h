#pragma once
#ifndef __HELPERS_RAW__
#include "HWindows.h"
#include "Singleton.hpp"
#include <functional>
#include <utility>
#include <vector>
#include <string>

namespace H {
	class CrashHandler {
	public:
		~CrashHandler() = default;
		CrashHandler(std::wstring runProtocol, std::wstring appCenterId, std::wstring appUuid);

		void SetProtocolCommandArgs(std::vector<std::pair<std::wstring, std::wstring>> protocolCommandArgs);
		void SetCrashCallback(std::function<void()> crashCallback);

	private:
		std::wstring runProtocol;
		std::function<void()> crashCallback = nullptr;
		std::vector<std::pair<std::wstring, std::wstring>> protocolCommandArgs;
	};
	
	using CrashHandlerSingleton = SingletonScoped<CrashHandler, std::wstring, std::wstring, std::wstring>;
}
#endif // __HELPERS_RAW__