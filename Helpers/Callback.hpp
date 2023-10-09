#pragma once
#include <memory>
#include <cassert>
#include "TokenContext.hpp"

template<class... Types>
class ICallback {
public:
	ICallback() {}
	virtual ~ICallback() = default;

	virtual void Invoke(Types... args) = 0;
	virtual ICallback* Clone() const = 0;
};


template<typename T, typename... Types>
class GenericCallback : public ICallback<Types...> {
public:

	GenericCallback(TokenContextWeak<T> ctx, void(*callbackFn)(typename TokenContextWeak<T>::Data_t* data, Types... args))
		: ctx(ctx)
		, callbackFn(callbackFn)
	{
	}

	GenericCallback(const GenericCallback& other)
		: ctx(other.ctx)
		, callbackFn(other.callbackFn)
	{
	}

	virtual ~GenericCallback() = default;

	GenericCallback& operator=(const GenericCallback& other) {
		if (this != &other) {
			this->ctx = other.ctx;
			this->callbackFn = other.callbackFn;
		}

		return *this;
	}

	GenericCallback& operator=(GenericCallback&& other) {
		if (this != &other) {
			this->ctx = std::move(other.ctx);
			this->callbackFn = std::move(other.callbackFn);
		}

		return *this;
	}

	void Invoke(Types... args) override {
		if (this->ctx.token.expired()) {
			return;
		}

		this->callbackFn(this->ctx.data, std::forward<Types>(args)...);
	}

	ICallback<Types...>* Clone() const override {
		GenericCallback* clone = new GenericCallback(*this);
		return clone;
	}

private:
	TokenContextWeak<T> ctx;
	void(*callbackFn)(typename TokenContextWeak<T>::Data_t* data, Types... args);
};

template<class... Types>
class Callback {
public:
	Callback()
	{
	}

	Callback(std::unique_ptr<ICallback<Types...>>&& callback)
		: callback(std::move(callback))
	{
	}

	Callback(const Callback& other)
		: callback(other.callback ? std::unique_ptr<ICallback<Types...>>(other.callback->Clone()) : nullptr)
	{
	}

	Callback(Callback&& other)
		: callback(std::move(other.callback))
	{
	}

	Callback& operator=(const Callback& other) {
		if (this != &other) {
			this->callback = other.callback ? std::unique_ptr<ICallback<Types...>>(other.callback->Clone()) : nullptr;
		}

		return *this;
	}

	Callback& operator=(Callback&& other) {
		if (this != &other) {
			this->callback = std::move(other.callback);
		}

		return *this;
	}

	void operator()(Types... args) {
		if (!this->callback) {
			assert(false && " --> callback is empty!");
			return;
		}
		this->callback->Invoke(args...);
	}

	operator bool() const {
		return this->callback != nullptr;
	}

private:
	std::unique_ptr<ICallback<Types...>> callback;
};


template<typename T, typename... Types>
Callback<Types...> MakeCallback(TokenContextWeak<T> ctx, void(*callbackFn)(typename TokenContextWeak<T>::Data_t* data, Types... args)) {
	auto icallback = std::make_unique<GenericCallback<T, Types...>>(ctx, callbackFn);
	return Callback<Types...>(std::move(icallback));
}