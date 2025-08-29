#pragma once
#include "Helpers/common.h"

#include <unordered_map>
#include <iterator>
#include <vector>
#include <ranges>

namespace STD_EXT_NS {
	namespace ranges {
		// Адаптер, как в C++23: to<std::vector>() - объект, который можно конкатенировать через |
		template <typename Container>
		struct to_container_adaptor {
			// pipe operator
			template <::std::ranges::input_range Range>
			friend Container operator|(Range&& r, to_container_adaptor) {
				Container container;

				if constexpr (requires { container.reserve(::std::ranges::size(r)); }) {
					container.reserve(::std::ranges::size(r));
				}

				// универсально для map/set и т.п.
				// inserter чуть медленнее для vector, зато компилируется везде
				::std::ranges::copy(r, ::std::inserter(container, container.end()));
				
				return container;
			}
		};


		template <typename Container>
		constexpr to_container_adaptor<Container> to() noexcept {
			return {};
		}
	}
}