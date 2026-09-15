#pragma once

#include <filesystem>
#include <vector>

namespace utility_helpers::new_helpers::filesystem {
    std::vector<unsigned char> ReadAllBytes(const std::filesystem::path& path);
}