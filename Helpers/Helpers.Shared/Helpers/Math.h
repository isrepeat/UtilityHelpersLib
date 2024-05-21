#pragma once
#include "common.h"

namespace HELPERS_NS {
	struct Size {
		unsigned int width;
		unsigned int height;

		Size() = default;

		Size(unsigned int width, unsigned int height)
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

	inline Size operator*(Size size, float value) {
		return Size{ 
			static_cast<unsigned int>(size.width * value),
			static_cast<unsigned int>(size.height * value),
		};
	}

	inline Size operator/(Size size, float value) {
		return Size{
			static_cast<unsigned int>(size.width / value),
			static_cast<unsigned int>(size.height / value),
		};
	}
}