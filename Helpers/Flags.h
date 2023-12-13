#pragma once
#include <type_traits>

namespace H {
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
		//operator bool() const { // for compare with boolean values (anyway need explicitly cast with boolean compare, so you need override boolean operators ==, != ...)
		//	return (bool)i;
		//}

		bool Has(UnderlyingType mask) const {
			return i & mask;
		}

	private:
		UnderlyingType i = 0;
	};
}