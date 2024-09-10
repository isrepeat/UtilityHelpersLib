#pragma once
#include "common.h"
#include "String.hpp"
#include <regex>
#include <vector>

namespace HELPERS_NS {
	// TODO: add template with args ...
	void DebugOutput(const std::wstring& msg);
	void DebugOutput(const std::string& msg);

	std::string to_lower(std::string str);
	std::string to_upper(std::string str);
}