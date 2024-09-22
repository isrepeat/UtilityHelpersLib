#pragma once
#include "SystemInfo.h"

#if COMPILE_FOR_DESKTOP
#include "Helpers/Helpers.h"
#include "Helpers/Logger.h"
#include "Helpers/Regex.h"

#define INFO_BUFFER_SIZE 32767

namespace HELPERS_NS {
    namespace details {
        Locale GetParsedLocaleFromLanguageTag(std::string localName) {
            H::Locale parsedLocale{
                .localName = localName,
            };
            auto matches = H::Regex::GetRegexMatches<char>(localName, std::regex{ H::Locale::regExToParseLocale });
            if (matches.size() > 0) {
                auto& match = matches[0];
                assert(match.capturedGroups.size() > 3);
                parsedLocale.languageCode = match.capturedGroups[1];
                parsedLocale.scriptCode = match.capturedGroups[2];
                parsedLocale.countryCode = match.capturedGroups[3];
            }
            return parsedLocale;
        }
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

    Locale GetUserDefaultLocale() {
        std::wstring localeName(LOCALE_NAME_MAX_LENGTH, L'\0');
        if (::GetUserDefaultLocaleName(localeName.data(), localeName.size())) {
            return details::GetParsedLocaleFromLanguageTag(H::WStrToStr(localeName));
        }
        return {};
    }

    std::vector<Locale> GetUserPreferredUILocales() {
        ULONG numLanguages;
        WCHAR languagesBuffer[256];
        ULONG bufferSize = sizeof(languagesBuffer) / sizeof(languagesBuffer[0]);

        std::vector<Locale> prefferedLanguages;
        if (::GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages, languagesBuffer, &bufferSize)) {
            WCHAR* languagesIt = languagesBuffer;
            for (int i = 0; i < numLanguages; i++) {
                prefferedLanguages.push_back(
                    details::GetParsedLocaleFromLanguageTag(H::WStrToStr(std::wstring(languagesIt)))
                );    
                // +1 to skip '\0' separator.
                languagesIt += prefferedLanguages.back().localName.size() + 1;
            }
        }
        return prefferedLanguages;
    }
}
#endif