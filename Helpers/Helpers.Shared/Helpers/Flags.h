#pragma once
#include "common.h"
#include <type_traits>

namespace HELPERS_NS {
    // TODO: add other binary operators with flags
	template <typename Enum>
	class Flags {
	public:
		using UnderlyingType = std::underlying_type_t<Enum>; // for most cases it is 'int'

		Flags() = default;
		Flags(int i) : i(i) {}
		Flags(const Enum& enm) : i((UnderlyingType)enm) {}


		Flags operator |= (UnderlyingType mask) {
			i |= mask;
			return *this;
		}
		Flags operator &= (UnderlyingType mask) {
			i &= mask;
			return *this;
		}
		Flags operator |= (Enum enm) {
			return *this |= (UnderlyingType)enm;
		}
		Flags operator &= (Enum enm) {
			return *this &= (UnderlyingType)enm;
		}

		Flags operator | (UnderlyingType mask) const {
			return Flags(i | mask);
		}
		Flags operator & (UnderlyingType mask) const {
			return Flags(i & mask);
		}
		Flags operator | (Enum enm) const {
			return *this | (UnderlyingType)enm;
		}
		Flags operator & (Enum enm) const {
			return *this & (UnderlyingType)enm;
		}
		
		operator UnderlyingType() const {
			return i;
		}
		operator Enum() const {
			return static_cast<Enum>(i);
		}
		//operator bool() const { // for compare with boolean values (anyway need explicitly cast with boolean compare, so you need override boolean operators ==, != ...)
		//	return (bool)i;
		//}

		bool Has(UnderlyingType mask) const {
			return i & mask;
		}
		bool Has(Enum mask) const {
			return Has((UnderlyingType)mask);
		}

		Enum ToEnum() {
			return operator Enum();
		}

		void ProcessAllFlags(std::function<bool(Enum)> handleCallback) {
			assert(handleCallback);
			if (!handleCallback)
				return;

			UnderlyingType currentBitPos = 1;
			do {
				if (currentBitPos & i) {
					if (handleCallback(static_cast<Enum>(currentBitPos))) {
						return;
					}
				}
			} while ((currentBitPos <<= 1) <= i);
		}

	private:
		UnderlyingType i = 0;
	};
}