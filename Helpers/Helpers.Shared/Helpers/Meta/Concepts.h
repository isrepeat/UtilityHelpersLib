#pragma once
#include "Helpers/common.h"
#include <concepts>

#if __cpp_concepts
namespace HELPERS_NS {
	namespace meta {
		namespace concepts {
			struct concept_placeholder_t {
			};
			inline constexpr concept_placeholder_t placeholder;

			template<concept_placeholder_t, bool TCondition>
			concept rule = TCondition;

			template<typename T>
			concept has_type = requires {
				typename T;
			};

			template<typename T>
			concept is_dereferenceable = requires(const T & val) {
				{ *val };
			};
		}
	}
}
#endif