#pragma once
#include <cstdint>
#include <string>

namespace {{application}}::application::core {
    // Значения являются частью JNI-контракта и не зависят от порядка элементов.
    enum class HostCommand : std::int32_t {
        requestApplicationUpdate = 1,
    };

    struct HostCommandData final {
        std::string value;
        std::string additionalValue;
    };
}

namespace {{application}}::application::interface {
    class IHostCommandDispatcher {
    public:
        virtual ~IHostCommandDispatcher() = default;
        virtual void Dispatch(core::HostCommand command, const core::HostCommandData& data = {}) = 0;
    };
}