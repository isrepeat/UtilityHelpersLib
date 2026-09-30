#pragma once

#include <string_view>
#include <fstream>
#include <mutex>

namespace {{application}}::android_host {
    class SessionLogger final {
    public:
        void Configure(std::string_view path);
        void Info(std::string_view category, std::string_view message);
        void Flush();

    private:
        std::mutex mutex;
        std::ofstream stream;
    };
}