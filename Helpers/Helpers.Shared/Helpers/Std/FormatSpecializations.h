#pragma once
#include "Helpers/common.h"

#include <filesystem>
#include <format>

#if !defined(__cpp_lib_format_path)
//
// ░ std::filesystem::path
// ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
//
template<>
struct std::formatter<std::filesystem::path, char> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}
	auto format(const std::filesystem::path& path, std::format_context& ctx) const {
		// Если нужна гарантированная UTF-8:
		// std::string s(reinterpret_cast<const char*>(path.u8string().c_str()));
		return std::format_to(ctx.out(), "{}", path.string());
	}
};

template<>
struct std::formatter<std::filesystem::path, wchar_t> {
	constexpr auto parse(std::wformat_parse_context& ctx) {
		return ctx.begin();
	}
	auto format(const std::filesystem::path& path, std::wformat_context& ctx) const {
		return std::format_to(ctx.out(), L"{}", path.wstring());
	}
};
#endif