#pragma once
#include "Macros.h"

// base class for simple destruction based scopes
template<class T>
struct ScopedPushBase {
	NO_COPY(ScopedPushBase);

	T *parent;

	ScopedPushBase() 
		: parent(nullptr)
	{
	}

	ScopedPushBase(ScopedPushBase &&other)
		: parent(std::move(other.parent))
	{
		other.parent = nullptr;
	}

	ScopedPushBase &operator=(ScopedPushBase &&other) {
		if (this != &other) {
			this->parent = std::move(other.parent);
			other.parent = nullptr;
		}

		return *this;
	}
};

// wrapper that initializes ScopedPushBaseImpl class
template<class T, class ScopedPushBaseImpl> 
struct ScopedPushWrapper {
	ScopedPushBaseImpl impl;

	ScopedPushWrapper(T *parent) {
		impl.parent = parent;
	}
};