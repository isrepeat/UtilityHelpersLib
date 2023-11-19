#pragma once // Include this file after spdlog headers
#include <filesystem>

template<>
struct fmt::formatter<std::filesystem::path, char> {
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.end();
    }
    auto format(const std::filesystem::path& path, format_context& ctx) -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "{}", path.string());
    }
};

template<>
struct fmt::formatter<std::filesystem::path, wchar_t> {
    constexpr auto parse(wformat_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.end();
    }
    auto format(const std::filesystem::path& path, wformat_context& ctx) -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), L"{}", path.wstring());
    }
};


#ifdef QT_CORE_LIB
#include <QString>

template<>
struct fmt::formatter<QString, char> {
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.end();
    }
    auto format(const QString& qtString, format_context& ctx) -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "{}", qtString.toStdString());
    }
};

template<>
struct fmt::formatter<QString, wchar_t> {
    constexpr auto parse(wformat_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.end();
    }
    auto format(const QString& qtString, wformat_context& ctx) -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), L"{}", qtString.toStdWString());
    }
};
#endif