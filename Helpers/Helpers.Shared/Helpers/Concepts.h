#pragma once
#include "common.h"

#if __cpp_concepts
namespace HELPERS_NS {
	namespace concepts {
		template <typename T>
		concept True = true;

		template<typename T>
		concept HasType = requires {
			typename T;
		};

		template<bool... _Concepts>
		concept Conjunction = (_Concepts && ...);

		// Concept: тип поддерживает operator* (shared_ptr, unique_ptr, optional и т.п.)
		template<typename T>
		concept Dereferenceable = requires(const T& val) {
			{ *val };
		};
	}
}
#endif