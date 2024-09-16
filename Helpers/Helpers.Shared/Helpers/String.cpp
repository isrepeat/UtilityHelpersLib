#pragma once
#include "String.h"
#include <Windows.h>

namespace HELPERS_NS {
	void DebugOutput(const std::wstring& msg) {
		OutputDebugStringW((L"[dbg] " + msg+L"\n").c_str());
	}

	void DebugOutput(const std::string& msg) {
		OutputDebugStringA(("[dbg] " + msg+"\n").c_str());
	}
}