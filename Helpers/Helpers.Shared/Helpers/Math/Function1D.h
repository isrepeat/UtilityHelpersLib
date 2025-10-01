#pragma once
#include "Helpers/common.h"
#include "Helpers/Meta/Concepts.h"

#if _HAS_CXX20
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <utility>

namespace HELPERS_NS {
	namespace Math {
		enum class FnForm {
			Src,
			Derivative1,
			Derivative2,
			Primitive1,
			Primitive2,
		};

		//
		// ░ Function1D
		// ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
		//
		// Trait который будет true если TFn это Derivative / Primitive.
		template <typename TFn>
		struct is_function1d_form : std::false_type {};

		class Function1D {
		public:
			using Fn_t = std::function<double(double)>;

		private:
			template <int N, typename TDerivative>
			struct Derivative {
				static_assert(N >= 1, "Derivative order must be >= 1.");
				Fn_t fn;

				explicit Derivative(TDerivative&& fn)
					// Конвертируем любой callable в std::function.
					// Здесь можно передать как rvalue-лямбду, так и std::function.
					: fn{ std::forward<TDerivative>(fn) } {
				}
			};

			template <int N, typename TPrimitive>
			struct Primitive {
				static_assert(N >= 1, "Primitive order must be >= 1.");
				Fn_t fn;

				explicit Primitive(TPrimitive&& fn)
					: fn{ std::forward<TPrimitive>(fn) } {
				}
			};

		public:
			// std::decay_t<TFn> нужен, чтобы нормализовать тип:
			// - убрать ссылки и cv-квалификаторы (если передадут lvalue-лямбду или const std::function&),
			// - превратить массивы/функции в указатели.
			// Это гарантирует, что шаблон Derivative/Primitive хранит "чистый" копируемый тип,
			// а не ссылку на временный объект.
			template <int N, typename TFn>
			static Derivative<N, std::decay_t<TFn>> MakeDerivative(TFn&& fn) {
				return Derivative<N, std::decay_t<TFn>>{ std::forward<TFn>(fn) };
			}

			template <int N, typename TFn>
			static Primitive<N, std::decay_t<TFn>> MakePrimitive(TFn&& fn) {
				return Primitive<N, std::decay_t<TFn>>{ std::forward<TFn>(fn) };
			}

			// Builder с requires: принимает только формы (Derivative<N> / Primitive<N>).
			template <typename... TFunctionForms>
			__requires_expr(
				is_function1d_form<std::decay_t<TFunctionForms>>::value && ...
			) static Function1D Make(
				Fn_t srcFunction,
				TFunctionForms&&... functionForms
			) {
				Function1D fn{ std::move(srcFunction) };
				(fn.AddFunctionForm(std::forward<TFunctionForms>(functionForms)), ...);
				return fn;
			}


			explicit Function1D(Fn_t srcFunction)
				: srcFunction{ std::move(srcFunction) } {
			}

			template <int N>
			Function1D& SetDerivative(Fn_t derivativeFunction) {
				static_assert(N >= 1, "Derivative order must be >= 1.");
				this->derivativesMap[N] = std::move(derivativeFunction);
				return *this;
			}

			template <int N>
			Function1D& SetPrimitive(Fn_t primitiveFunction) {
				static_assert(N >= 1, "Primitive order must be >= 1.");
				this->primitivesMap[N] = std::move(primitiveFunction);
				return *this;
			}


			template <int N>
			bool HasDerivative() const {
				static_assert(N >= 1, "Derivative order must be >= 1.");
				return this->derivativesMap.find(N) != this->derivativesMap.end();
			}

			template <int N>
			bool HasPrimitive() const {
				static_assert(N >= 1, "Primitive order must be >= 1.");
				return this->primitivesMap.find(N) != this->primitivesMap.end();
			}


			double Invoke(double x) const {
				return this->srcFunction(x);
			}

			template <int N>
			double InvokeDerivative(double x) const {
				static_assert(N >= 1, "Derivative order must be >= 1.");
				const auto it = this->derivativesMap.find(N);

				if (it == this->derivativesMap.end()) {
					throw std::logic_error{ "Derivative<N> is not set." };
				}
				return it->second(x);
			}

			template <int N>
			double InvokePrimitive(double x) const {
				static_assert(N >= 1, "Primitive order must be >= 1.");
				const auto it = this->primitivesMap.find(N);

				if (it == this->primitivesMap.end()) {
					throw std::logic_error{ "Primitive<N> is not set." };
				}
				return it->second(x);
			}


			class CallProxy {
			public:
				explicit CallProxy(
					const Function1D& functionOwner,
					const FnForm fnForm
				)
					: functionOwner{ &functionOwner }
					, fnForm{ fnForm } {
				}

				double operator()(double x) const {
					switch (this->fnForm) {
					case FnForm::Src:
						return this->functionOwner->Invoke(x);

					case FnForm::Derivative1:
						return this->functionOwner->InvokeDerivative<1>(x);

					case FnForm::Derivative2:
						return this->functionOwner->InvokeDerivative<2>(x);

					case FnForm::Primitive1:
						return this->functionOwner->InvokePrimitive<1>(x);

					case FnForm::Primitive2:
						return this->functionOwner->InvokePrimitive<2>(x);

					default:
						throw std::logic_error("could not find fnForm");
					};
				}

			private:
				const Function1D* functionOwner;
				FnForm fnForm;
			}; // class CallProxy


			double operator()(double x) const {
				return this->Invoke(x);
			}
			
			CallProxy operator[](const FnForm fnForm) const {
				return Function1D::CallProxy{ *this, fnForm };
			}

		private:
			template <int N, typename TDerivative>
			void AddFunctionForm(Derivative<N, TDerivative>&& derivative) {
				this->SetDerivative<N>(std::move(derivative.fn));
			}

			template <int N, typename TPrimitive>
			void AddFunctionForm(Primitive<N, TPrimitive>&& primitive) {
				this->SetPrimitive<N>(std::move(primitive.fn));
			}

		private:
			Fn_t srcFunction;
			std::unordered_map<int, Fn_t> derivativesMap;
			std::unordered_map<int, Fn_t> primitivesMap;
		};

		template <int N, typename TFn>
		struct is_function1d_form<Function1D::Derivative<N, TFn>> : std::true_type {};

		template <int N, typename TFn>
		struct is_function1d_form<Function1D::Primitive<N, TFn>> : std::true_type {};


#if HELPERS_ENABLE_COMPILETIME_TESTS // == 0
		//
		// ░ Tests
		// ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
		//
		namespace Function1DTests {
			consteval void TestMeta() {
			}

			consteval void TestAll() {
				TestMeta();
			}

			constexpr int __function1D_tests_anchor = (TestAll(), 0);
		}
#endif // HELPERS_ENABLE_COMPILETIME_TESTS
	}
}
#endif