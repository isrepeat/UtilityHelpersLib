#pragma once
#include "SystemInfo.h"
#if COMPILE_FOR_DESKTOP
#include "Helpers.h"
#include "Logger.h"

#define INFO_BUFFER_SIZE 32767

namespace HELPERS_NS {
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

    std::wstring GetUserDefaultLocaleName() {
        std::wstring localeName(LOCALE_NAME_MAX_LENGTH, L'\0');
        if (::GetUserDefaultLocaleName(localeName.data(), localeName.size())) {
            return localeName;
        }
        return {};
    }

    std::vector<std::wstring> GetUserPreferredUILanguages() {
        ULONG numLanguages;
        WCHAR languagesBuffer[256];
        ULONG bufferSize = sizeof(languagesBuffer) / sizeof(languagesBuffer[0]);

        std::vector<std::wstring> prefferedLanguages;
        if (::GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages, languagesBuffer, &bufferSize)) {
            WCHAR* languagesIt = languagesBuffer;
            for (int i = 0; i < numLanguages; i++) {
                prefferedLanguages.push_back(std::wstring(languagesIt));
                languagesIt += prefferedLanguages.back().size() + 1;
            }
        }
        return prefferedLanguages;
    }
}
#endif