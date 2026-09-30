#include "SessionLogger.h"

#include <stdexcept>
#include <iomanip>
#include <chrono>
#include <string>
#include <ctime>

namespace {{application}}::android_host {
    void SessionLogger::Configure(std::string_view path) {
        std::scoped_lock lock(this->mutex);
        this->stream.close();
        this->stream.open(std::string(path), std::ios::out | std::ios::app);
        if (!this->stream) {
            throw std::runtime_error("Cannot open session log");
        }
    }

    void SessionLogger::Info(std::string_view category, std::string_view message) {
        std::scoped_lock lock(this->mutex);
        if (!this->stream) {
            return;
        }
        const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm localTime{};
#if defined(_WIN32)
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif
        this->stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S")
            << " [INFO] [" << category << "] " << message << '\n';
    }

    void SessionLogger::Flush() {
        std::scoped_lock lock(this->mutex);
        this->stream.flush();
    }
}