#include "HelpersNew/Filesystem/ReadAllBytes.h"

#include <fstream>
#include <stdexcept>

namespace utility_helpers::new_helpers::filesystem {
    std::vector<unsigned char> ReadAllBytes(const std::filesystem::path& path) {
        std::ifstream stream(path, std::ios::binary | std::ios::ate);

        if (!stream) {
            throw std::runtime_error("Cannot open file: " + path.string());
        }

        const std::streamsize length = stream.tellg();

        if (length < 0) {
            throw std::runtime_error("Cannot determine file size: " + path.string());
        }

        std::vector<unsigned char> data(static_cast<size_t>(length));
        stream.seekg(0, std::ios::beg);

        if (!data.empty() && !stream.read(reinterpret_cast<char*>(data.data()), length)) {
            throw std::runtime_error("Cannot read file: " + path.string());
        }

        return data;
    }
}