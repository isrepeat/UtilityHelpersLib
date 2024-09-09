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


    std::string to_lower(std::string str) {
        for (char& ch : str) {
            ch = tolower(ch);
        }
        return str;
    }

    std::string to_upper(std::string str) {
        for (char& ch : str) {
            ch = toupper(ch);
        }
        return str;
    }
}