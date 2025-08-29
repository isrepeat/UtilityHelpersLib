#pragma once
#include "Helpers/common.h"

#include <algorithm>
#include <vector>
#include <ranges>

//
//  ░ Range adaptor pattern (C++23-стиль)
//  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
//
//  Мы используем единый паттерн для расширений std::ranges::views:
//
//  • <name>_closure  — «замыкание-адаптер»: хранит параметры вызова (если есть) и
//    объявляет friend-оператор `operator|` (hidden friend idiom).
//    Hidden friend даёт доступ к приватным данным closure и участвует в ADL,
//    поэтому выражение `range | <name>(...)` корректно находит нужный оператор.
//
//  • <name>_fn       — функциональный объект с реализацией операции над range.
//    Его удобно иметь, когда есть внутренняя логика помимо самого «пайпинга»
//    (например, вычисление countToTake в drop_last_fn). В простых случаях (как to)
//    отдельный *_fn не обязателен.
//
//  • Публичная фабрика <name>(...) возвращает <name>_closure.
//
//  Hidden friend idiom:
//  - Объявляем operator| прямо внутри <name>_closure как friend.
//  - Он остаётся "свободной" функцией (не членом),
//    но имеет доступ к приватным данным и подхватывается ADL
//    (она объявлена внутри класса, и потому компилятор «привязывает» её 
//    к тому же namespace, где объявлен класс).

namespace STD_EXT_NS {
	namespace ranges {
		namespace views {
			//
			// ░ to
			// ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
			//
			template <typename TContainer>
			struct to_closure {
				template <typename TRange>
					requires ::std::ranges::input_range<TRange>
				friend auto operator|(
					TRange&& range,
					to_closure /*self*/
					) {
					TContainer container;

					if constexpr (requires { container.reserve(::std::ranges::size(range)); }) {
						container.reserve(::std::ranges::size(range));
					}

					// универсально для map/set и т.п.
					// inserter чуть медленнее для vector, зато компилируется везде
					::std::ranges::copy(range, ::std::inserter(container, container.end()));

					return container;
				}
			};

			template <typename TContainer>
			constexpr to_closure<TContainer> to() noexcept {
				return {};
			}

			//
			// ░ drop_last
			// ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
			//
			struct drop_last_fn {
				::std::size_t countToDrop;

				template <typename TRange>
					requires ::std::ranges::forward_range<TRange>
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

			struct drop_last_closure {
			private:
				::std::size_t countToDrop;

			public:
				explicit drop_last_closure(::std::size_t n)
					: countToDrop{ n } {
				}

				template <typename TRange>
					requires ::std::ranges::forward_range<TRange>
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