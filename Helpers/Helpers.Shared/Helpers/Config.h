#pragma once
#include "common.h"
#include "ThreadSafeObject.hpp"
#include "TypeTraits.hpp"
#include "Singleton.hpp"

namespace HELPERS_NS {
	template <typename T>
	struct ConfigData {
		static_assert(HELPERS_NS::dependent_false<T>::value, "You must use specialization");
		struct Data {
		};
	};

	template <typename DerivedT>
	class ConfigBase : public HELPERS_NS::Singleton<typename DerivedT> {
	private:
		using _MyBase = HELPERS_NS::Singleton<typename DerivedT>;

	public:
		using _Base = ConfigBase<DerivedT>;
		using _Data = typename ConfigData<DerivedT>::Data;

		ConfigBase(const _Data& data)
			: data{ data }
		{}
		virtual ~ConfigBase() = default;

		static HELPERS_NS::ThreadSafeObject<std::recursive_mutex, const _Data>::_Locked GetDataLocked() {
			return _MyBase::GetInstance().data.Lock();
		}
		static _Data GetDataCopy() {
			auto locked = GetDataLocked();
			auto copy = locked.Get();
			return copy;
		}

	private:
		HELPERS_NS::ThreadSafeObject<std::recursive_mutex, const _Data> data;
	};
}