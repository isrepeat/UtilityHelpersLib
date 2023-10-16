#pragma once
#include "String.hpp"
#include <regex>
#include <vector>

namespace H {
	// TODO: add template with args ...
	void DebugOutput(const std::wstring& msg);
	void DebugOutput(const std::string& msg);

	std::string to_lower(std::string str);
	std::string to_upper(std::string str);

	// [Regex]
	struct RegexMatchResult {
		RegexMatchResult(const std::wsmatch& match);
		std::vector<std::wstring> matches;
		std::wstring preffix;
		std::wstring suffix;
	};

	std::vector<RegexMatchResult> GetRegexMatches(const std::wstring& text, const std::wregex& rx);
	bool FindInsideTagWithRegex(const std::wstring& text, const std::wstring& tag, const std::wregex& innerRx);
	bool FindInsideAnyTagWithRegex(const std::wstring& text, const std::wregex& innerRx);
}