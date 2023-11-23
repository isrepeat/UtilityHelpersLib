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

		Flags operator |= (Enum enm) const {
			i |= (UnderlyingType)enm; 
			return *this;
		}
		Flags operator | (Enum enm) const {
			return Flags(i | (UnderlyingType)enm);
		}
		Flags operator & (Enum enm) const {
			return Flags(i & (UnderlyingType)enm);
		}
		Flags operator & (UnderlyingType mask) const {
			return Flags(i & mask);
		}

		operator UnderlyingType() const {
			return i;
		}
		//operator bool() const { // for compare with boolean values (anyway need explicitly cast with boolean compare, so you need override boolean operators ==, != ...)
		//	return (bool)i;
		//}

	private:
		UnderlyingType i = 0;
	};
}