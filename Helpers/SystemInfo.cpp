#pragma once
#include "SystemInfo.h"
#include "Helpers.h"
#include "Logger.h"

#define INFO_BUFFER_SIZE 32767

namespace H {
    std::wstring GetUserNameW() {
        DWORD bufCharCount = INFO_BUFFER_SIZE;
        std::wstring userName(INFO_BUFFER_SIZE, '\0');

        if (!::GetUserNameW(userName.data(), &bufCharCount)) {
            LogLastError;
        }

        userName.resize(bufCharCount);
        return userName;
    }

	std::wstring GetComputerNameW() {
        DWORD bufCharCount = INFO_BUFFER_SIZE;
        std::wstring cumputerName(INFO_BUFFER_SIZE, '\0');

        if (!::GetComputerNameW(cumputerName.data(), &bufCharCount)) {
            LogLastError;
        }

        cumputerName.resize(bufCharCount);
        return cumputerName;
	}
}