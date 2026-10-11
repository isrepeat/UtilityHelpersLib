#pragma once

#include "./IEvent.h"

#include <type_traits>
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <memory>
#include <vector>
#include <mutex>

namespace utility_helpers::new_helpers::event {
    template<typename TSignature>
    class Signal;

    // Синхронная рассылка: обработчики выполняются в потоке Invoke, в порядке подписки.
    // Параллельные Invoke могут одновременно вызвать один обработчик.
    // Disconnect не ожидает завершения уже начавшегося вызова.
    template<typename... TArgs>
    class Signal<void(TArgs...)> final : public IEvent<void(TArgs...)> {
        static_assert((!std::is_rvalue_reference_v<TArgs> && ...), "Signal cannot broadcast rvalue references");
        static_assert(((std::is_reference_v<TArgs> || std::is_copy_constructible_v<TArgs>) && ...),
            "Signal value arguments must be copyable");

        struct Slot final : detail::Subscription {
            explicit Slot(typename IEvent<void(TArgs...)>::Handler handler)
                : handler(std::move(handler)) {
            }
            typename IEvent<void(TArgs...)>::Handler handler;
        };

        struct State {
            std::mutex mutex;
            std::vector<std::weak_ptr<Slot>> slots;
        };

    public:
        using Handler = typename IEvent<void(TArgs...)>::Handler;
        Signal() = default;
        ~Signal() override {
            this->DisconnectAll();
        }
        Signal(const Signal&) = delete;
        Signal& operator=(const Signal&) = delete;
        Signal(Signal&&) = delete;
        Signal& operator=(Signal&&) = delete;

        [[nodiscard]] Connection Subscribe(Handler handler) override {
            if (!handler) {
                throw std::invalid_argument("Signal handler must not be empty");
            }
            auto slot = std::make_shared<Slot>(std::move(handler));
            slot->lifetime = this->state;
            {
                std::lock_guard<std::mutex> lock(this->state->mutex);
                auto& slots = this->state->slots;
                slots.erase(std::remove_if(slots.begin(), slots.end(), [](const auto& weakSlot) {
                    return weakSlot.expired();
                }), slots.end());
                slots.emplace_back(slot);
            }
            return Connection(std::move(slot));
        }

        // Новые подписки участвуют только в следующем Invoke; исключения передаются вызывающему.
        void Invoke(TArgs... args) {
            const auto state = this->state;
            std::vector<std::shared_ptr<Slot>> snapshot;
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                for (const auto& weakSlot : state->slots) {
                    if (auto slot = weakSlot.lock()) {
                        snapshot.push_back(std::move(slot));
                    }
                }
            }
            for (const auto& slot : snapshot) {
                if (slot->connected.load()) {
                    slot->handler(args...);
                }
            }
        }

        void DisconnectAll() {
            // Освобождаем захваты обработчиков вне блокировки: деструкторы могут вызвать Signal.
            std::vector<std::shared_ptr<Slot>> snapshot;
            {
                std::lock_guard<std::mutex> lock(this->state->mutex);
                for (const auto& weakSlot : this->state->slots) {
                    if (auto slot = weakSlot.lock()) {
                        snapshot.push_back(std::move(slot));
                    }
                }
                this->state->slots.clear();
                for (const auto& slot : snapshot) {
                    slot->connected.store(false);
                }
            }
        }

    private:
        std::shared_ptr<State> state = std::make_shared<State>();
    };
}