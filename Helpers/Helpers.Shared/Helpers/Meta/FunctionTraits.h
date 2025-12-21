#pragma once
#include "Helpers/common.h"
#include "Concepts.h"
#include <type_traits>
#include <tuple>

namespace HELPERS_NS {
    namespace meta {
        //
        // ░ Implemantation
        // ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
        //
        namespace impl {
            //
            // ░ Category dispatch
            //		
            struct MemFunPtrTag {};
            // Тип попадает сюда, если это УКАЗАТЕЛЬ НА ЧЛЕН-ФУНКЦИЮ класса:
            //   TRet (C::*)(Args...) [cv/ref/noexcept]
            // Включает в себя и указатели на operator() внутри класса (у функторов/лямбд),
            // потому что `decltype(&T::operator())` всегда имеет вид pointer-to-member.

            struct FunPtrTag {};
            // Сюда попадает свободная C-style функция или static-метод:
            //   TRet (*)(Args...) [noexcept]

            struct CallableTag {};
            // Сюда попадает САМ тип класса/union, у которого есть operator():
            //   struct F { void operator()() const; };
            // Здесь T = F (а не указатель на метод). Для таких типов мы отдельно берём
            // decltype(&T::operator()) и повторно анализируем его как MemFunPtrTag.

            struct UnknownTag {};
            // Всё остальное, что не распознано (не функция, не указатель, не callable тип).

            template <typename T>
            using category_t =
                std::conditional_t<std::is_member_function_pointer_v<T>, MemFunPtrTag,
                std::conditional_t<std::is_pointer_v<T>&& std::is_function_v<std::remove_pointer_t<T>>, FunPtrTag,
                std::conditional_t<std::is_class_v<T> || std::is_union_v<T>, CallableTag,
                UnknownTag>>>;

            //
            // ░ FunctionTraitsBaseArgs
            //
            template <typename TRet, typename... A>
            struct FunctionTraitsBaseArgs {
                enum { ArgumentCount = sizeof...(A) };
                using Ret = TRet;
                using Arguments = std::tuple<A...>;

                template <std::size_t i>
                struct arg {
                    using type = typename std::tuple_element<i, std::tuple<A...>>::type;
                };

                template <std::size_t i>
                struct arg_val {
                    using type = std::remove_pointer_t<std::remove_reference_t<typename arg<i>::type>>;
                };
            };

            //
            // ░ FunctionTraitsImpl
            //
            template <typename Sig, typename Tag>
            struct FunctionTraitsImpl; // специализации ниже

            //
            // ░ select_call_operator
            // ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
            // 
            // Явная сигнатура для перегруженного operator().
            //
            template <typename C, typename TSignature>
            struct select_call_operator;

            template <typename C, typename TRet, typename... A>
            struct select_call_operator<C, TRet(A...)> {
                using type = TRet(C::*)(A...);
            };

            template <typename C, typename TRet, typename... A>
            struct select_call_operator<C, TRet(A...) const> {
                using type = TRet(C::*)(A...) const;
            };

            template <typename C, typename TRet, typename... A>
            struct select_call_operator<C, TRet(A...)&> {
                using type = TRet(C::*)(A...)&;
            };

            template <typename C, typename TRet, typename... A>
            struct select_call_operator<C, TRet(A...) const&> {
                using type = TRet(C::*)(A...) const&;
            };

            template <typename C, typename TRet, typename... A>
            struct select_call_operator<C, TRet(A...)&&> {
                using type = TRet(C::*)(A...)&&;
            };

            template <typename C, typename TRet, typename... A>
            struct select_call_operator<C, TRet(A...) const&&> {
                using type = TRet(C::*)(A...) const&&;
            };

            template <typename C, typename TRet, typename... A>
            struct select_call_operator<C, TRet(A...) noexcept> {
                using type = TRet(C::*)(A...) noexcept;
            };

            template <typename C, typename TRet, typename... A>
            struct select_call_operator<C, TRet(A...) const noexcept> {
                using type = TRet(C::*)(A...) const noexcept;
            };

            template <typename C, typename TRet, typename... A>
            struct select_call_operator<C, TRet(A...) & noexcept> {
                using type = TRet(C::*)(A...) & noexcept;
            };

            template <typename C, typename TRet, typename... A>
            struct select_call_operator<C, TRet(A...) const& noexcept> {
                using type = TRet(C::*)(A...) const& noexcept;
            };

            template <typename C, typename TRet, typename... A>
            struct select_call_operator<C, TRet(A...) && noexcept> {
                using type = TRet(C::*)(A...) && noexcept;
            };

            template <typename C, typename TRet, typename... A>
            struct select_call_operator<C, TRet(A...) const&& noexcept> {
                using type = TRet(C::*)(A...) const&& noexcept;
            };

            template <typename C, typename TSignature>
            using select_call_operator_t = typename select_call_operator<C, TSignature>::type;

            // Impl-local callable-detection helpers (used by impl::FunctionTraitsCallable)
            template <typename T, typename = void>
            struct has_non_overloaded_call_op_impl : std::false_type {};

            template <typename T>
            struct has_non_overloaded_call_op_impl<T, std::void_t<decltype(&T::operator())>> : std::true_type {};

            template <typename T>
            inline constexpr bool has_non_overloaded_call_op_impl_v = has_non_overloaded_call_op_impl<T>::value;

            template <typename C, typename TSignature, typename = void>
            struct has_call_op_with_signature_impl : std::false_type {};

            template <typename C, typename TSignature>
            struct has_call_op_with_signature_impl<C, TSignature, std::void_t<decltype(static_cast<select_call_operator_t<C, TSignature>>(&C::operator()))>> : std::true_type {};

            template <typename C, typename TSignature>
            inline constexpr bool has_call_op_with_signature_impl_v = has_call_op_with_signature_impl<C, TSignature>::value;

            //
            // ░ FunctionTraitsCallable
            //
            template <typename T>
            struct FunctionTraitsCallable {
                static_assert(
                    has_non_overloaded_call_op_impl_v<T>,
                    "operator() is overloaded or templated; use FunctionTraitsWithSignature<T,Sig>"
                    );

                using FunctionTraitsImpl_t = FunctionTraitsImpl<decltype(&T::operator()), MemFunPtrTag>;
            };
        } // namespace impl

        // Callable-detection traits available to both frontends.
        namespace concepts {
#if __cpp_concepts
            // Concepts delegate to the impl helpers defined above (keeps implementation centralized).
            template <typename T>
            concept has_non_overloaded_call_op = impl::has_non_overloaded_call_op_impl_v<T>;

            template <typename C, typename TSignature>
            concept has_call_op_with_signature = impl::has_call_op_with_signature_impl_v<C, TSignature>;

            template <typename T>
            inline constexpr bool has_non_overloaded_call_op_v = has_non_overloaded_call_op<T>;

            template <typename C, typename TSignature>
            inline constexpr bool has_call_op_with_signature_v = has_call_op_with_signature<C, TSignature>;
#else
            // C++17: expose the impl helpers directly as convenience variable templates.
            template <typename T>
            inline constexpr bool has_non_overloaded_call_op_v = impl::has_non_overloaded_call_op_impl_v<T>;

            template <typename C, typename TSignature>
            inline constexpr bool has_call_op_with_signature_v = impl::has_call_op_with_signature_impl_v<C, TSignature>;
#endif
        } // namespace concepts

        //
        // ░ API
        // ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
        //
        enum class FuncKind { Functor, CstyleFunc, ClassMember };
        enum class RefQual { None, LValue, RValue };

        //
        // ░ impl::FunctionTraitsImpl specializations
        //
#define CV_EMPTY
#define CV_CONST const
#define REF_NONE
#define REF_LREF &
#define REF_RREF &&
#define NOEXCEPT_EMPTY
#define NOEXCEPT_SPEC noexcept

#define HFT_DEFINE_MEMPTR_TRAITS(CV, REF, NOEXCEPT, IS_CONST_BOOL, REF_QUAL_ENUM, IS_NOEXCEPT_BOOL) \
        template <typename TRet, typename C, typename... A> \
        struct impl::FunctionTraitsImpl<TRet (C::*)(A...) CV REF NOEXCEPT, impl::MemFunPtrTag> \
            : impl::FunctionTraitsBaseArgs<TRet, A...> \
        { \
            using Class = C; \
            using Signature = TRet (C::*)(A...) CV REF NOEXCEPT; \
            static constexpr bool IsConst = IS_CONST_BOOL; \
            static constexpr bool IsNoexcept = IS_NOEXCEPT_BOOL; \
            static constexpr RefQual Ref = REF_QUAL_ENUM; \
        };

#define HFT_DEFINE_FUNPTR_TRAITS(NOEXCEPT, IS_NOEXCEPT_BOOL) \
        template <typename TRet, typename... A> \
        struct impl::FunctionTraitsImpl<TRet (*)(A...) NOEXCEPT, impl::FunPtrTag> \
            : impl::FunctionTraitsBaseArgs<TRet, A...> \
        { \
            using Class = void; \
            using Signature = TRet (*)(A...) NOEXCEPT; \
            static constexpr bool IsConst = false; \
            static constexpr bool IsNoexcept = IS_NOEXCEPT_BOOL; \
            static constexpr RefQual Ref = RefQual::None; \
        };

        // 12 member-комбинаций
        HFT_DEFINE_MEMPTR_TRAITS(CV_EMPTY, REF_NONE, NOEXCEPT_EMPTY, false, RefQual::None, false);
        HFT_DEFINE_MEMPTR_TRAITS(CV_CONST, REF_NONE, NOEXCEPT_EMPTY, true, RefQual::None, false);
        HFT_DEFINE_MEMPTR_TRAITS(CV_EMPTY, REF_NONE, NOEXCEPT_SPEC, false, RefQual::None, true);
        HFT_DEFINE_MEMPTR_TRAITS(CV_CONST, REF_NONE, NOEXCEPT_SPEC, true, RefQual::None, true);

        HFT_DEFINE_MEMPTR_TRAITS(CV_EMPTY, REF_LREF, NOEXCEPT_EMPTY, false, RefQual::LValue, false);
        HFT_DEFINE_MEMPTR_TRAITS(CV_CONST, REF_LREF, NOEXCEPT_EMPTY, true, RefQual::LValue, false);
        HFT_DEFINE_MEMPTR_TRAITS(CV_EMPTY, REF_LREF, NOEXCEPT_SPEC, false, RefQual::LValue, true);
        HFT_DEFINE_MEMPTR_TRAITS(CV_CONST, REF_LREF, NOEXCEPT_SPEC, true, RefQual::LValue, true);

        HFT_DEFINE_MEMPTR_TRAITS(CV_EMPTY, REF_RREF, NOEXCEPT_EMPTY, false, RefQual::RValue, false);
        HFT_DEFINE_MEMPTR_TRAITS(CV_CONST, REF_RREF, NOEXCEPT_EMPTY, true, RefQual::RValue, false);
        HFT_DEFINE_MEMPTR_TRAITS(CV_EMPTY, REF_RREF, NOEXCEPT_SPEC, false, RefQual::RValue, true);
        HFT_DEFINE_MEMPTR_TRAITS(CV_CONST, REF_RREF, NOEXCEPT_SPEC, true, RefQual::RValue, true);

        // 2 free-комбинации
        HFT_DEFINE_FUNPTR_TRAITS(NOEXCEPT_EMPTY, false);
        HFT_DEFINE_FUNPTR_TRAITS(NOEXCEPT_SPEC, true);

#undef HFT_DEFINE_MEMPTR_TRAITS
#undef HFT_DEFINE_FUNPTR_TRAITS
#undef CV_EMPTY
#undef CV_CONST
#undef REF_NONE
#undef REF_LREF
#undef REF_RREF
#undef NOEXCEPT_EMPTY
#undef NOEXCEPT_SPEC

#if __cpp_concepts
        // C++20 concepts frontend; trait *implementations* are defined above in the common area (no-op here).

        //
        // ░ FunctionTraits: Primary template
        //
        // По category_t<TFn> маршрутизирует в одну из реализаций:
        // - MemFunPtrTag для TRet (C::*)(...) [cv/ref/noexcept]
        // - FunPtrTag для TRet (*)(...) [noexcept]
        // - CallableTag/UnknownTag (для них реализаций нет) => перехватывает специализация ниже
        template <typename TFn>
        struct FunctionTraits : impl::FunctionTraitsImpl<TFn, impl::category_t<TFn>> {
            static constexpr FuncKind Kind = std::is_member_function_pointer_v<TFn>
                ? FuncKind::ClassMember
                : FuncKind::CstyleFunc;
        };

        //
        // ░ FunctionTraits: callable-тип (класс/union с одиночным operator())
        //
        template <typename TFn>
        __requires_expr((
            std::is_class_v<TFn> ||
            std::is_union_v<TFn>
            ) &&
            concepts::has_non_overloaded_call_op_v<TFn>
        ) struct FunctionTraits<TFn> : impl::FunctionTraitsCallable<TFn>::FunctionTraitsImpl_t {
            static constexpr FuncKind Kind = FuncKind::Functor;
        };

        //
        // ░ FunctionTraitsWithSignature: Primary template
        //
        template <typename TFn, typename TSignature>
        struct FunctionTraitsWithSignature {
        };

        //
        // ░ FunctionTraitsWithSignature: для перегруженного operator().
        //
        template <typename TFn, typename TSignature>
        __requires_expr(
            concepts::has_call_op_with_signature_v<TFn, TSignature>
        ) struct FunctionTraitsWithSignature<TFn, TSignature> : FunctionTraits<impl::select_call_operator_t<TFn, TSignature>> {
        };

        namespace concepts {
            template<typename TFn>
            concept has_method =
                FunctionTraits<TFn>::Kind == FuncKind::ClassMember;

            template <typename TFn, typename TSignature>
            concept has_method_with_signature =
                has_method<TFn> &&
                std::is_same_v<typename FunctionTraits<TFn>::Signature, TSignature>;

            template<typename TFn>
            concept has_static_function =
                FunctionTraits<TFn>::Kind == FuncKind::CstyleFunc;

            template<typename TFn, typename TSignature>
            concept has_static_function_with_signature =
                has_static_function<TFn> &&
                std::is_same_v<typename FunctionTraits<TFn>::Signature, TSignature>;
        } // namespace concepts
#else
        // Pre-C++20 (C++17) fallback: detection traits are already defined in the common area above.

        // Primary template variant for pre-C++20 (adds Enable param so callable specialization can be a partial specialization)
        template <typename TFn, typename Enable = void>
        struct FunctionTraits : impl::FunctionTraitsImpl<TFn, impl::category_t<TFn>> {
            static constexpr FuncKind Kind = std::is_member_function_pointer<TFn>::value
                ? FuncKind::ClassMember
                : FuncKind::CstyleFunc;
        };

        // callable-type partial-specialization (functors) for pre-C++20
        template <typename TFn>
        struct FunctionTraits<TFn, std::enable_if_t<
            (std::is_class<TFn>::value || std::is_union<TFn>::value) &&
            concepts::has_non_overloaded_call_op_v<TFn>
            >> : impl::FunctionTraitsCallable<TFn>::FunctionTraitsImpl_t {
            static constexpr FuncKind Kind = FuncKind::Functor;
        };

        // FunctionTraitsWithSignature primary + specialization for pre-C++20
        template <typename TFn, typename TSignature, typename Enable = void>
        struct FunctionTraitsWithSignature {
        };

        template <typename TFn, typename TSignature>
        struct FunctionTraitsWithSignature<TFn, TSignature, std::enable_if_t<
            concepts::has_call_op_with_signature_v<TFn, TSignature>
            >> : FunctionTraits<impl::select_call_operator_t<TFn, TSignature>> {
        };

        // Provide concept-like boolean traits for pre-C++20 so existing tests that use
        // `concepts::has_*` names continue to compile.
        namespace concepts {
            template <typename TFn>
            inline constexpr bool has_method = FunctionTraits<TFn>::Kind == FuncKind::ClassMember;

            template <typename TFn, typename TSignature>
            inline constexpr bool has_method_with_signature =
                has_method<TFn> && std::is_same_v<typename FunctionTraits<TFn>::Signature, TSignature>;

            template <typename TFn>
            inline constexpr bool has_static_function = FunctionTraits<TFn>::Kind == FuncKind::CstyleFunc;

            template <typename TFn, typename TSignature>
            inline constexpr bool has_static_function_with_signature =
                has_static_function<TFn> && std::is_same_v<typename FunctionTraits<TFn>::Signature, TSignature>;
        } // namespace concepts
#endif
    }
}