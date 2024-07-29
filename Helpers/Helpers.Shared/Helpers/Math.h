#pragma once
#include "common.h"

namespace HELPERS_NS {
	namespace details {
		//
		// Size
		//
		template <typename T>
		struct Size {
			T width;
			T height;

			Size()
				: width{ 0 }
				, height{ 0 }
			{}

			Size(T width, T height)
				: width{ width }
				, height{ height }
			{}

			bool operator == (const Size& other) const {
				return this->width == other.width && this->height == other.height;
			}
			bool operator != (const Size& other) const {
				return !(*this == other);
			}

			template <typename T>
			explicit operator Size<T>() {
				return Size<T>{
					static_cast<T>(this->width),
					static_cast<T>(this->height)
				};
			}
		};

		template <typename T>
		inline Size<T> operator*(Size<T> size, float value) {
			return Size<T>{
				static_cast<T>(size.width * value),
				static_cast<T>(size.height * value)
			};
		}

		template <typename T>
		inline Size<T> operator/(Size<T> size, float value) {
			return Size<T>{
				static_cast<T>(size.width / value),
				static_cast<T>(size.height / value)
			};
		}


		//
		// Rect
		//
		template <typename T>
		struct Rect {
			T left;
			T top;
			T right;
			T bottom;

			Rect()
				: left{ 0 }
				, top{ 0 }
				, right{ 0 }
				, bottom{ 0 }
			{}

			Rect(T left, T top, T right, T bottom )
				: left{ left }
				, top{ top }
				, right{ right }
				, bottom{ bottom }
			{}

			template <typename T>
			explicit operator Rect<T>() {
				return Rect<T>{
					static_cast<T>(this->left),
					static_cast<T>(this->top),
					static_cast<T>(this->right),
					static_cast<T>(this->bottom)
				};
			}
		};

	} // namespace details

	using Size = details::Size<unsigned int>;
	using Size_f = details::Size<float>;

	using Rect = details::Rect<unsigned int>;
	using Rect_f = details::Rect<float>;
}