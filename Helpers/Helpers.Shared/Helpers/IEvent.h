#pragma once
#include "common.h"
#include <functional>

template<typename ...Args>
class IEvent {
public:
	typedef std::function<void(Args...)> callback;

	virtual ~IEvent() = default;

	virtual std::shared_ptr<callback> Add(callback func) = 0;
};
