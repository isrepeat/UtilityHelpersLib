#pragma once

#include "./Connection.h"

#include <functional>

namespace utility_helpers::new_helpers::event {
    template<typename TSignature>
    class IEvent;

    // Подписчики получают только доступ к подписке, без возможности вызвать событие.
    template<typename... TArgs>
    class IEvent<void(TArgs...)> {
    public:
        using Handler = std::function<void(TArgs...)>;
        virtual ~IEvent() = default;
        [[nodiscard]] virtual Connection Subscribe(Handler handler) = 0;
    };
}