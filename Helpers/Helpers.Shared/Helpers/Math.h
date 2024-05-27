#pragma once
#include "common.h"

namespace HELPERS_NS {
	namespace details {
		template <typename T>
		struct Size {
			T width;
			T height;

			Size() = default;

			Size(T width, T height)
				: width{ width }
				, height{ height }
			{}

			bool operator == (const Size& other) const {
				return width == other.width && height == other.height;
			}
			bool operator != (const Size& other) const {
				return !(*this == other);
			}
		};

		template <typename T>
		inline Size<T> operator*(Size<T> size, float value) {
			return Size<T>{
				static_cast<T>(size.width * value),
				static_cast<T>(size.height * value),
			};
		}

		template <typename T>
		inline Size<T> operator/(Size<T> size, float value) {
			return Size<T>{
				static_cast<T>(size.width / value),
				static_cast<T>(size.height / value),
			};
		}
	} // namespace details

	using Size = details::Size<unsigned int>;
	using Size_f = details::Size<float>;
}