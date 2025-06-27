#pragma once
#include "common.h"
#include "Concepts.h"
#include <type_traits>
#include <memory>

_STD_BEGIN
/* --------------------------------------------------- */
/* "type_identity" support for languages before c++20  */
/* --------------------------------------------------- */
#if _HAS_CXX20 == 0
template <class _Ty>
struct type_identity {
	using type = _Ty;
};
template <class _Ty>
using type_identity_t = typename type_identity<_Ty>::type;
#endif // _HAS_CXX20

_STD_END

namespace HELPERS_NS {
	 struct nothing {};

	 template<typename T> struct dependent_false : std::false_type {};

     template <typename T>
     struct ExtractUnderlyingType {
         using type = T;
     };

     template <template<typename> typename ContainerT, typename ItemT>
     struct ExtractUnderlyingType<ContainerT<ItemT>> {
         using type = ItemT;
     };

	 class PointerGetter {
	 public:
		 template<class T>
		 static T* Get(T* v) {
			 return v;
		 }

		 template<class T>
		 static T* Get(T& v) {
			 return &v;
		 }

		 template<class T>
		 static T* Get(std::unique_ptr<T>& v) {
			 return v.get();
		 }

		 template<class T>
		 static T* Get(std::shared_ptr<T>& v) {
			 return v.get();
		 }
	 };


	 class ValueExtractor {
	 private:
		 template<typename T>
		 struct GetFinalType {
			 using type = T;
		 };

		 template<typename T>
			 requires concepts::Dereferenceable<T>
		 struct GetFinalType<T> {
			 using type = typename GetFinalType<
				 std::remove_reference_t<decltype(*std::declval<T>())>
			 >::type;
		 };

	 public:
		 template<typename T>
		 static const typename GetFinalType<T>::type& From(const T& value) {
			 if constexpr (concepts::Dereferenceable<T>) {
				 return ValueExtractor::From(*value);
			 }
			 else {
				 return value;
			 }
		 }
	 };
}