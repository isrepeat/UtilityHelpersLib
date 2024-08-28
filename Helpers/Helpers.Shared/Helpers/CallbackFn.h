#pragma once
#include "common.h"
#include "Callback.hpp"

#include <functional>

template<typename R, typename... Ts>
class CallbackFn final : public ICallback<R, Ts...> {
public:
    CallbackFn(std::function<R(Ts...)> fn)
        : fn(std::move(fn))
    {}

    R Invoke(Ts... args) override {
        return this->fn(args...);
    }

    ICallback<R, Ts...>* Clone() const override {
        return new CallbackFn<R, Ts...>(this->fn);
    }

private:
    std::function<R(Ts...)> fn;
};

template<typename R, typename... Ts>
Callback<R, Ts...> MakeCallbackFn(std::function<R(Ts...)> fn) {
    auto icallback = std::make_unique<CallbackFn<R, Ts...>>(std::move(fn));
    return Callback<R, Ts...>(std::move(icallback));
}

template<typename Fn>
auto MakeCallbackFn(Fn fn) {
    return MakeCallbackFn(std::function(fn));
}
