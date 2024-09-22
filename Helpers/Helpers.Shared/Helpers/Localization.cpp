#pragma once
#include "Localization.h"

#if COMPILE_FOR_DESKTOP
#include "Helpers/FileSystem_inline.h"
#include "Helpers/Helpers.h"
#include "Helpers/String.h"
#include "Helpers/Logger.h"
#include "Helpers/Regex.h"
#include <fstream>

namespace HELPERS_NS {
    Locale::operator bool() const {
        return !this->localName.empty() || !this->languageCode.empty();
    }

    Locale Locale::ToLower() {
        return Locale{
            .localName = H::ToLower(this->localName),
            .languageCode = H::ToLower(this->languageCode),
            .scriptCode = H::ToLower(this->scriptCode),
            .countryCode = H::ToLower(this->countryCode),
        };
    }

    void Locale::Log() {
        LOG_DEBUG_D(".localName = {}", this->localName);
        LOG_DEBUG_D(".languageCode = {}", this->languageCode);
        if (!this->scriptCode.empty()) {
            LOG_DEBUG_D(".scriptCode = {}", this->scriptCode);
        }
        if (!this->countryCode.empty()) {
            LOG_DEBUG_D(".countryCode = {}", this->countryCode);
        }
    }
}

namespace STD_EXT_NS {
	namespace details {
		int FindFileCodePageInFirstLineOfStream(::std::ifstream& fileStream) {
			if (!fileStream.is_open()) {
				throw std::ex::fstream_not_opened{};
			}

			auto prevFileStreamPointerPos = fileStream.tellg();
			fileStream.seekg(0, std::ios::beg);

			std::string firstLine;
			if (std::getline(fileStream, firstLine)) {
				auto matches = H::Regex::GetRegexMatches(firstLine, std::regex("CodePage\\s*=\\s*(\\d+)"));
				if (!matches.empty()) {
					if (matches[0].capturedGroups.size() > 1) {
						// Do not recover file stream pointer pos.
						return std::stoi(matches[0].capturedGroups[1]);
					}
				}
			}

			fileStream.seekg(prevFileStreamPointerPos, std::ios::beg);
			return -1;
		}
	}
	ifstream::Data ifstream::ReadData() {
		if (!this->is_open()) {
			throw std::ex::fstream_not_opened{};
		}
		auto& ifstreamRef = static_cast<::std::ifstream&>(*this);

		Data data;
		auto codePage = details::FindFileCodePageInFirstLineOfStream(ifstreamRef);
		if (codePage != -1) {
			data.codePage = codePage;
		}
		data.byteArray = ::std::vector<char>{
			::std::istreambuf_iterator<char>(ifstreamRef),
			::std::istreambuf_iterator<char>()
		};
		return data;
	}
}
#endif