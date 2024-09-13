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

// Extends class with "Try()" method which can be used inside SAFE_RESULT macro
#define SafeObject(T) \
	T* Try() { \
		if (this == nullptr) { \
			__LOG_NULLPTR_OBJECT(T); \
			throw ex::std::bad_pointer(); \
		} \
		return this; \
	}


namespace HELPERS_NS {
	namespace ex {
		namespace std {
			struct bad_pointer {};

			template <typename T>
			struct unique_ptr : public ::std::unique_ptr<T> {
				using _MyBase = ::std::unique_ptr<T>;
				using _MyBase::unique_ptr;

				unique_ptr<T>& Try() {
					if (this == nullptr) {
						throw ex::std::bad_pointer{};
					}
					if (this->get() == nullptr) {
						throw ex::std::bad_pointer{};
					}
					return *this;
				}
			};

			template <typename T>
			struct shared_ptr : public ::std::shared_ptr<T> {
				using _MyBase = ::std::shared_ptr<T>;
				using _MyBase::shared_ptr;

				shared_ptr<T>& Try() {
					if (this == nullptr) {
						throw ex::std::bad_pointer{};
					}
					if (this->get() == nullptr) {
						throw ex::std::bad_pointer{};
					}
					return *this;
				}
			};

			template <typename T>
			struct weak_ptr : public ::std::weak_ptr<T> {
				using _MyBase = ::std::weak_ptr<T>;
				using _MyBase::weak_ptr;

				shared_ptr<T> Try() {
					if (this == nullptr) {
						throw ex::std::bad_pointer{};
					}
					auto sharedPtr = this->lock();
					if (sharedPtr == nullptr) {
						throw ex::std::bad_pointer{};
					}
					return sharedPtr;
				}
			};

			// Also you can expose std methods to be available outside through this namespace.
			//using ::std::make_unique;
		}
	}

	inline bool SafeConditionResult(std::function<bool()> fnCondition) {
		try {
			return static_cast<bool>(fnCondition());
		}
		catch (ex::std::bad_pointer&) {
			return false;
		}
		catch (...) {
			return false;
		}
	}

#define SAFE_RESULT(condition) HELPERS_NS::SafeConditionResult([&] { return condition;})
}