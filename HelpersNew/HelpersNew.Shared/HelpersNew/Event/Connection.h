#pragma once

#include <atomic>
#include <memory>
#include <utility>

namespace utility_helpers::new_helpers::event {
    namespace detail {
        struct Subscription {
            virtual ~Subscription() = default;
            std::atomic<bool> connected{true};
            std::weak_ptr<void> lifetime;
        };
    }

    // Единственный владелец подписки; уничтожение и перемещение отключают старую подписку.
    class Connection final {
    public:
        Connection() = default;
        explicit Connection(std::shared_ptr<detail::Subscription> subscription)
            : subscription(std::move(subscription)) {
        }
        ~Connection() {
            this->Disconnect();
        }
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        Connection(Connection&& connection) noexcept = default;
        Connection& operator=(Connection&& connection) noexcept {
            if (this != &connection) {
                this->Disconnect();
                this->subscription = std::move(connection.subscription);
            }
            return *this;
        }
        void Disconnect() noexcept {
            if (this->subscription) {
                this->subscription->connected.store(false);
            }
        }
        bool IsConnected() const noexcept {
            return this->subscription && !this->subscription->lifetime.expired()
                && this->subscription->connected.load();
        }

    private:
        std::shared_ptr<detail::Subscription> subscription;
    };
}