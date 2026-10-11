#include <HelpersNew/Event/Signal.h>

#include <stdexcept>
#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace event = utility_helpers::new_helpers::event;

void Check(bool value) {
    if (!value) {
        throw std::runtime_error("Signal test failed");
    }
}

int main() {
    event::Signal<void(std::string)> signal;
    std::vector<std::string> values;
    auto first = signal.Subscribe([&](std::string value) { values.push_back(value); });
    auto second = signal.Subscribe([&](std::string value) { values.push_back(value); });
    signal.Invoke("payload");
    Check(values == std::vector<std::string>{"payload", "payload"});
    first = std::move(second);
    Check(!second.IsConnected());
    signal.Invoke("next");
    Check(values.size() == 3);
    first.Disconnect();
    signal.Invoke("ignored");
    Check(values.size() == 3);
    {
        auto temporary = signal.Subscribe([&](std::string) { values.push_back("unexpected"); });
    }
    signal.Invoke("ignored");
    Check(values.size() == 3);

    event::Connection survivor;
    {
        event::Signal<void()> local;
        survivor = local.Subscribe([] {});
        Check(survivor.IsConnected());
    }
    Check(!survivor.IsConnected());
    survivor.Disconnect();

    event::Signal<void()> changes;
    event::Connection later;
    event::Connection added;
    int calls = 0;
    auto earlier = changes.Subscribe([&] {
        later.Disconnect();
        if (!added.IsConnected()) {
            added = changes.Subscribe([&] { ++calls; });
        }
    });
    later = changes.Subscribe([&] { calls += 100; });
    changes.Invoke();
    Check(calls == 0);
    changes.Invoke();
    Check(calls == 1);
    changes.DisconnectAll();
    Check(!earlier.IsConnected() && !added.IsConnected());

    event::Signal<void()> recursive;
    int depth = 0;
    auto recursion = recursive.Subscribe([&] {
        if (++depth == 1) {
            recursive.Invoke();
        }
    });
    recursive.Invoke();
    Check(depth == 2);

    event::Signal<void()> disconnecting;
    int disconnectedCalls = 0;
    auto disconnectAll = disconnecting.Subscribe([&] { disconnecting.DisconnectAll(); });
    auto skipped = disconnecting.Subscribe([&] { ++disconnectedCalls; });
    disconnecting.Invoke();
    Check(disconnectedCalls == 0 && !disconnectAll.IsConnected() && !skipped.IsConnected());

    event::Signal<void(int&)> references;
    event::IEvent<void(int&)>& subscriberAccess = references;
    auto reference = subscriberAccess.Subscribe([](int& value) { ++value; });
    int value = 4;
    references.Invoke(value);
    Check(value == 5);

    event::Signal<void()> exceptions;
    bool caught = false;
    try {
        auto empty = exceptions.Subscribe({});
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    Check(caught);
    auto throwing = exceptions.Subscribe([] { throw std::runtime_error("handler"); });
    caught = false;
    try {
        exceptions.Invoke();
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught);

    event::Signal<void()> concurrent;
    std::atomic<int> count{0};
    auto persistent = concurrent.Subscribe([&] { ++count; });
    std::thread publisher([&] {
        for (int i = 0; i < 10000; ++i) {
            concurrent.Invoke();
        }
    });
    for (int i = 0; i < 10000; ++i) {
        auto temporary = concurrent.Subscribe([] {});
    }
    publisher.join();
    Check(count == 10000);
}