#pragma once
#include <Helpers/common.h>
#include <string>

namespace CppFeatures {
	// TODO: Rewrite this project to use helpers without .cso files
	namespace details {
		inline std::string WStrToStr(const std::wstring& wstr, int codePage = CP_UTF8) {
			if (wstr.size() == 0)
				return std::string{};

			int sz = WideCharToMultiByte(codePage, 0, wstr.c_str(), -1, 0, 0, 0, 0);
			std::string res(sz, 0);
			WideCharToMultiByte(codePage, 0, wstr.c_str(), -1, &res[0], sz, 0, 0);
			return res.c_str(); // To delete '\0' use .c_str()
		}

		inline std::wstring StrToWStr(const std::string& str, int codePage = CP_UTF8) {
			if (str.size() == 0)
				return std::wstring{};

			int sz = MultiByteToWideChar(codePage, 0, str.c_str(), -1, 0, 0);
			std::wstring res(sz, 0);
			MultiByteToWideChar(codePage, 0, str.c_str(), -1, &res[0], sz);
			return res.c_str();
		}
	}
}