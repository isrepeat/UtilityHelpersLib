#pragma once
#include "common.h"

#if COMPILE_FOR_DESKTOP
#include <string_view>
#include <filesystem>
#include <optional>
#include <fstream>
#include <string>
#include <vector>

namespace HELPERS_NS {
	struct Locale {
		// https://stackoverflow.com/questions/8758340/is-there-a-regex-to-test-if-a-string-is-for-a-locale
		static constexpr const char* regExToParseLocale = "^([A-Za-z]{2,4})(?:[_-]([A-Za-z]{4}))?(?:[_-]([A-Za-z]{2}|[0-9]{3}))?$";
		
		std::string localName;    // "zh-Hant-TW" [group 0]			    | full locale name.
		std::string languageCode; // "zh"         [group 1]			    | 2 or 3, or 4 for future use, alpha.
		std::string scriptCode;   // "Hant"       [group 2] [optional]  | 4 alpha.
		std::string countryCode;  // "TW"         [group 3] [optional]  | 2 alpha or 3 digit.

		operator bool() const;

		Locale ToLower();
		void Log();
	};
}

namespace STD_EXT_NS {
	struct fstream_not_opened : ::std::exception {
		fstream_not_opened()
			: ::std::exception("stream not opened")
		{}
	};

	struct ifstream : public ::std::ifstream {
		using ::std::ifstream::basic_ifstream;

		struct Data {
			::std::vector<char> byteArray;
			::std::optional<int> codePage;
		};

		Data ReadData();
	};
}
#endif