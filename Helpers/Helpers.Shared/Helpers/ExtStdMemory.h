#pragma once
#include "common.h"
#include <string_view>
#include <functional>
#include <memory>

#if defined(ENABLE_NULLPTR_LOGGING)
#include "Logger.h"
#define __LOG_NULLPTR_OBJECT(T) LOG_DEBUG_D("'{}' is nullptr", typeid(T).name())
#else
#define __LOG_NULLPTR_OBJECT(T)
#endif

// TODO: If this header included twice may be problems with 'STD_EXT_NS' macro. Fix it.
// Extends class with "Try()" method which can be used inside SAFE_RESULT macro
#define SafeObject(T) \
	T* Try() { \
		if (this == nullptr) { \
			__LOG_NULLPTR_OBJECT(T); \
			throw STD_EXT_NS::bad_pointer(); \
		} \
		return this; \
	}


namespace STD_EXT_NS {
	struct bad_pointer : ::std::exception {
		bad_pointer()
			: ::std::exception("bad pointer")
		{}
	};

	template <typename T>
	struct unique_ptr : public ::std::unique_ptr<T> {
		PP_FORWARD_CTOR(unique_ptr, ::std::unique_ptr<T>);

		unique_ptr<T>& Try() {
			if (this == nullptr) {
				throw STD_EXT_NS::bad_pointer{};
			}
			if (this->get() == nullptr) {
				throw STD_EXT_NS::bad_pointer{};
			}
			return *this;
		}
	};

	template <typename T>
	struct shared_ptr : public ::std::shared_ptr<T> {
		PP_FORWARD_CTOR(shared_ptr, ::std::shared_ptr<T>);

		shared_ptr<T>& Try() {
			if (this == nullptr) {
				throw STD_EXT_NS::bad_pointer{};
			}
			if (this->get() == nullptr) {
				throw STD_EXT_NS::bad_pointer{};
			}
			return *this;
		}
	};

	template <typename T>
	struct weak_ptr : public ::std::weak_ptr<T> {
		PP_FORWARD_CTOR(weak_ptr, ::std::weak_ptr<T>);

		shared_ptr<T> Try() {
			if (this == nullptr) {
				throw STD_EXT_NS::bad_pointer{};
			}
			auto sharedPtr = this->lock();
			if (sharedPtr == nullptr) {
				throw STD_EXT_NS::bad_pointer{};
			}
			return sharedPtr;
		}
	};
}

namespace HELPERS_NS {
	inline bool SafeConditionResult(std::function<bool()> fnCondition) {
		try {
			return static_cast<bool>(fnCondition());
		}
		catch (...) {
			return false;
		}
	}

#define SAFE_RESULT(condition) HELPERS_NS::SafeConditionResult([&] { return condition;})
}