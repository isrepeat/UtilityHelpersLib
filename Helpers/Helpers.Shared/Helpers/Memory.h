#pragma once
#include "common.h"
#include <memory>

namespace HELPERS_NS {
	// https://codereview.stackexchange.com/questions/256651/dynamic-pointer-cast-for-stdunique-ptr
	// https://stackoverflow.com/questions/26377430/how-to-perform-a-dynamic-cast-with-a-unique-ptr
	template <typename To, typename From, typename Deleter>
	std::unique_ptr<To, Deleter> dynamic_unique_cast(std::unique_ptr<From, Deleter>&& ptr) {
		if (To* cast = dynamic_cast<To*>(ptr.get())) {
			std::unique_ptr<To, Deleter> result(cast, std::move(ptr.get_deleter()));
			ptr.release();
			return result;
		}
		return {};
	}

	template <typename To, typename From>
	std::unique_ptr<To> dynamic_unique_cast(std::unique_ptr<From>&& ptr) {
		if (To* cast = dynamic_cast<To*>(ptr.get())) {
			std::unique_ptr<To> result(cast);
			ptr.release();
			return result;
		}
		return {};
	}


	template<class T, class D>
	std::unique_ptr<T, D> WrapUnique(T* ptr, const D& deleter) {
		return std::unique_ptr<T, D>(ptr, deleter);
	}

	template <typename SmartPointerT>
	SmartPointerT& EmptyPointerRef() {
		static std::remove_reference_t<SmartPointerT> emptyPoiner = nullptr;
		return emptyPoiner;
	}


	template<class T, class D>
	class GetAddressOfUnique {
	public:
		GetAddressOfUnique(std::unique_ptr<T, D>& ptr)
			: ptr(ptr)
		{
			this->rawPtr = this->ptr.release();
		}

		operator T** () {
			return &this->rawPtr;
		}

		~GetAddressOfUnique() {
			this->ptr.reset(this->rawPtr);
		}

	private:
		T* rawPtr;
		std::unique_ptr<T, D>& ptr;
	};

	template<class T, class D>
	GetAddressOfUnique<T, D> GetAddressOf(std::unique_ptr<T, D>& v) {
		return GetAddressOfUnique<T, D>(v);
	}

	template<class T>
	struct CoDeleter {
		void operator()(T* ptr) {
			CoTaskMemFree(ptr);
		}
	};

	// Unique pointer for objects that were allocated with CoTaskMemAlloc
	template<class T>
	struct CoUniquePtr : public std::unique_ptr<T, CoDeleter<T>> {
		GetAddressOfUnique<T, CoDeleter<T>> GetAddressOf() {
			return GetAddressOfUnique<T, CoDeleter<T>>(*this);
		}
	};


	//
	// UniquePointer specialization
	// 
	template<class T>
	struct UniquePointer {
		static constexpr std::string_view templateNotes = "Primary template";
	};

	template<class T>
	struct DoubleArrayDeleter {
		DoubleArrayDeleter(int allocatedSize)
			: allocatedSize{ allocatedSize }
		{}

		void operator()(T** pptr) {
			std::for_each(pptr, pptr + this->allocatedSize, std::default_delete<T[]>());
			delete[] pptr;
		}

	private:
		int allocatedSize = 0;
	};

	template<class T>
	struct UniquePointer<T*> : public std::unique_ptr<T*, DoubleArrayDeleter<T>> {
		static constexpr std::string_view templateNotes = "Specialized for <T*>";
		using _MyBase = std::unique_ptr<T*, DoubleArrayDeleter<T>>;

		UniquePointer(int size)
			: _MyBase(new T* [size] {}, size)
		{}
	};


	//
	// ViewPointer specialization
	// 
	template<class T>
	struct ViewPointer {
		static constexpr std::string_view templateNotes = "Primary template";
	};

	template<class T>
	struct DoubleArrayViewDeleter {
		void operator()(T** pptr) {
			// We don't delete internal pointers because their lifetime is managed by another class.
			delete[] pptr;
		}
	};

	template<class T>
	struct ViewPointer<T*> : public std::unique_ptr<T*, DoubleArrayViewDeleter<T>> {
		static constexpr std::string_view templateNotes = "Specialized for <T*>";
		using _MyBase = std::unique_ptr<T*, DoubleArrayViewDeleter<T>>;

		ViewPointer(int size)
			: _MyBase(new T* [size] {})
		{}
	};
}