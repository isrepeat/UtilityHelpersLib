#pragma once
#include "Helpers/common.h"
#include "Helpers/Meta/Utilities/ValueExtractor.h"
#include "Helpers/Meta/Concepts.h"

#include <algorithm>
#include <concepts>
#include <iterator>
#include <utility> 
#include <vector>
#include <set>

namespace HELPERS_NS {
#if __cpp_concepts
	namespace meta {
		namespace concepts {
			// У контейнера есть erase(first,last) и он возвращает iterator
			template <typename TContainer>
			concept is_erasable_container = requires (
				TContainer c,
				typename TContainer::iterator f,
				typename TContainer::iterator l
				) {
					{ c.erase(f, l) } -> std::same_as<typename TContainer::iterator>;
			};
		}
	}
#endif

	namespace Collection {
		//
		// ░ Extract
		// ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
		//
		template<
			typename TIter,
			typename TContainer,
			typename TPredicate
		>
#if __cpp_concepts
			requires meta::concepts::rule<
				meta::concepts::placeholder,
					// Итератор принадлежит контейнеру
					std::same_as<TIter, typename TContainer::iterator> and
					meta::concepts::is_erasable_container<TContainer>
			>
#endif
		TContainer Extract(
			TContainer& collection,
			TIter firstIt,
			TIter lastIt,
			TPredicate&& predicate
		) {
			// Сдвигаем "под удаление" в хвост
			auto firstRemovedIt = std::remove_if(firstIt, lastIt, std::forward<TPredicate>(predicate));

			// Перемещаем хвост в результирующий контейнер.
			TContainer extractedItems;

			extractedItems.insert(
				extractedItems.end(),
				std::make_move_iterator(firstRemovedIt),
				std::make_move_iterator(lastIt)
			);

			// Удаляем хвост из исходного контейнера.
			collection.erase(firstRemovedIt, lastIt);
			return extractedItems;
		}


		template<
			typename TContainer,
			typename TPredicate
		>
#if __cpp_concepts
		requires meta::concepts::is_erasable_container<TContainer>
#endif
		TContainer Extract(TContainer& collection, TPredicate&& predicate) {
			return Extract(
				collection,
				collection.begin(),
				collection.end(),
				std::forward<TPredicate>(predicate)
			);
		}


		//
		// ░ Set
		// ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
		//
		class Set {
		public:
			template<typename T, typename... TArgs>
			static bool IsHasIntersection(
				const std::set<T, TArgs...>& a,
				const std::set<T, TArgs...>& b
			) {
				// Альтернатива через std::set_intersection, но это менее эффективно, т.к. создаёт временный буфер.

				if (a.empty() || b.empty()) {
					return false;
				}

				auto itA = a.begin();
				auto itB = b.begin();

				while (itA != a.end() && itB != b.end()) {
					// Универсально извлекаем значения из it, разыменовывая любую вложенность
					const auto& valA = meta::ValueExtractor::From(itA);
					const auto& valB = meta::ValueExtractor::From(itB);

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