#pragma once
#include "Helpers/common.h"
#include "Helpers/Meta/Concepts.h"

#include <algorithm>
#include <ranges>

namespace STD_EXT_NS {
	namespace ranges {
		namespace views {
			//
			// ░ drop_last
			// ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
			//
			// ░ drop_last_fn
			//
			struct drop_last_fn {
				::std::size_t countToDrop;

				template <typename TRange>
				__requires requires { requires
					::std::ranges::forward_range<TRange>;
				}
				auto operator() (
					TRange&& range
					) const {
					// Общее количество элементов
					const auto total = ::std::ranges::distance(range);

					// Сколько реально можем отбросить (не больше, чем есть элементов)
					const auto safeDrop = static_cast<::std::size_t>(
						::std::min<::std::ptrdiff_t>(
							total,
							static_cast<::std::ptrdiff_t>(this->countToDrop)
						));

					// Сколько элементов нужно оставить
					const ::std::size_t countToTake = static_cast<::std::size_t>(
						::std::max<::std::ptrdiff_t>(
							total - static_cast<::std::ptrdiff_t>(safeDrop),
							0
						));

					// Возвращаем view на первые countToTake элементов
					return ::std::ranges::views::take(::std::forward<TRange>(range), countToTake);
				}
			};


			//
			// ░ drop_last_closure
			//
			struct drop_last_closure {
			private:
				::std::size_t countToDrop;

			public:
				explicit drop_last_closure(::std::size_t n)
					: countToDrop{ n } {
				}

				template <typename TRange>
				__requires requires { requires
					::std::ranges::forward_range<TRange>;
				}
				friend auto operator|(
					TRange&& range,
					const drop_last_closure& self
					) {
					return drop_last_fn{ self.countToDrop }(::std::forward<TRange>(range));
				}
			};

			inline drop_last_closure drop_last(::std::size_t n) {
				return drop_last_closure{ n };
			}
		}
	}

	namespace views = ranges::views;
}