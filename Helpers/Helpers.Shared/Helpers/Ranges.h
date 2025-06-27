#pragma once
#include "common.h"
#include "TypeTraits.hpp"
#include "Concepts.h"
#include <algorithm>
#include <vector>
#include <set>

#if !__cpp_concepts
#pragma message(PP_MSG_ERROR_REQUIRE_CPP_CONCEPTS)
#endif

namespace HELPERS_NS {
	namespace Ranges {
		class Set {
		public:
			template<typename T, typename... Args>
			static bool IsHasIntersection(const std::set<T, Args...>& a, const std::set<T, Args...>& b) {
				// Альтернатива через std::set_intersection, но это менее эффективно, т.к. создаёт временный буфер.

				if (a.empty() || b.empty()) {
					return false;
				}

				auto itA = a.begin();
				auto itB = b.begin();

				while (itA != a.end() && itB != b.end()) {
					// Универсально извлекаем значения из it, разыменовывая любую вложенность
					const auto& valA = ValueExtractor::From(itA);
					const auto& valB = ValueExtractor::From(itB);

					if (valA < valB) {
						++itA;
					}
					else if (valB < valA) {
						++itB;
					}
					else {
						// Найден общий элемент
						return true;
					}
				}

				return false;
			}
		};
	}
}