#pragma once
#include "common.h"
#include "FunctionTraits.hpp"

#if __cpp_concepts
namespace HELPERS_NS {
	namespace concepts {
		template <typename T>
		concept True = true;

		template<typename T>
		concept HasType = requires {
			typename T;
		};

		template<typename Fn>
		concept HasMethod =
			H::FunctionTraits<Fn>::Kind == H::FuncKind::ClassMember &&
			H::FunctionTraits<Fn>::IsPointerToMemberFunction == true;

		template<typename Fn, typename Signature>
		concept HasMethodWithSignature =
			H::FunctionTraits<Fn>::Kind == H::FuncKind::ClassMember &&
			H::FunctionTraits<Fn>::IsPointerToMemberFunction == true &&
			std::is_same_v<typename H::FunctionTraits<Fn>::Function, Signature>;

		template<typename Fn>
		concept HasStaticMethod =
			H::FunctionTraits<Fn>::IsPointerToMemberFunction == false;

		template<typename Fn, typename Signature>
		concept HasStaticMethodWithSignature =
			H::FunctionTraits<Fn>::IsPointerToMemberFunction == false &&
			std::is_same_v<typename H::FunctionTraits<Fn>::Function, Signature>;

		template<bool... _Concepts>
		concept Conjunction = (_Concepts && ...);
	}
}
#endif