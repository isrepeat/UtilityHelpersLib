#pragma once
#include "Helpers/common.h"
#include "Helpers/Meta/Concepts.h"
#include "Tools.h"

#include <type_traits>
#include <algorithm>
#include <iterator>
#include <optional>
#include <utility>
#include <ranges>

namespace STD_EXT_NS {
	namespace ranges {
		namespace views {
			//
			// ░ concat
			// ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
			//
			// Идея:
			//   Лениво склеиваем ДВА произвольных viewable_range так, чтобы можно было писать:
			//     rng1 | std::ex::ranges::views::concat(rng2)
			//   Адаптер ничего не копирует из элементов, просто "прочитав" первый диапазон,
			//   продолжает итерацию вторым. Жизненный цикл обоих исходных представлений (views)
			//   хранится внутри concat_view, поэтому итераторы безопасны.
			//
			// Ограничения (requires):
			//   - Оба аргумента — input_range и уже "представления" (view), чтобы их можно было хранить по значению.
			//   - Ссылочные типы элементов совместимы через common_reference_with: у concat_view должен быть единый
			//     type reference, который подходит и для V1::reference, и для V2::reference.
			//
			// Замечания по реализации:
			//   - Категория итератора: input_iterator (достаточно для большинства наших сценариев).
			//     При желании можно нарастить поддержку forward_range и т.п.
			//   - Конец диапазона — default_sentinel. Итератор == end тогда и только тогда, когда
			//     мы уже перешли ко второй части И дошли до её конца.
			//   - Если первая часть пустая, begin() сразу начинает со второй.
			//
			// ░ concat_view
			//
			template <
				typename V1,
				typename V2
			>
				requires HELPERS_NS::meta::concepts::rule<
					HELPERS_NS::meta::concepts::placeholder,
						::std::ranges::input_range<V1> and
						::std::ranges::input_range<V2> and
						::std::ranges::view<V1> and
						::std::ranges::view<V2> and
						::std::common_reference_with<
						::std::ranges::range_reference_t<V1>,
						::std::ranges::range_reference_t<V2>
						>
				>
			class concat_view : public ::std::ranges::view_interface<concat_view<V1, V2>> {
			public:
				class iterator;
				friend class iterator; // доступ к приватным полям viewFirst / viewSecond.

				concat_view() = default;

				constexpr concat_view(
					V1 v1,
					V2 v2
				)
					: viewFirst{ ::std::move(v1) }
					, viewSecond{ ::std::move(v2) } {
				}

				//
				// ░ concat_view::iterator
				//
				// Держит ссылку на родителя (для доступа к viewFirst/viewSecond) и состояние:
				//   - inFirst          : мы сейчас в первой части?
				//   - iterFirstOpt     : активный итератор первой части (если inFirst == true)
				//   - iterSecondOpt    : активный итератор второй части (если inFirst == false)
				//
				// Ленивая "стыковка":
				//   - ++ на первой части двигает iterFirst; при достижении end(first) — переключаемся
				//     на начало второй части (inFirst = false, iterSecondOpt = begin(second)).
				//   - ++ на второй части просто двигает iterSecond.
				//
				class iterator {
					using It1_t = ::std::ranges::iterator_t<V1>;
					using It2_t = ::std::ranges::iterator_t<V2>;

				public:
					// NOTE:
					// Эти имена объявлены по соглашениям итераторов C++ и распознаются стандартной библиотекой.
					// - Мир ranges опирается на iterator_concept/difference_type.
					// - Классическая STL (iterator_traits) смотрит на value_type/iterator_category.
					using iterator_concept = ::std::input_iterator_tag;
					using difference_type = ::std::ptrdiff_t;

					// Единый ссылочный тип (для operator*):
					using reference = ::std::common_reference_t<
						::std::ranges::range_reference_t<V1>,
						::std::ranges::range_reference_t<V2>
					>;

					// value_type нужен для indirectly_readable => iter_value_t<It>.
					// Берём общий "значенческий" тип элементов обеих частей.
					using value_type = ::std::common_type_t<
						::std::ranges::range_value_t<V1>,
						::std::ranges::range_value_t<V2>
					>;

					// Для старых алгоритмов (не обязателен для ranges, но не мешает):
					using iterator_category = ::std::input_iterator_tag;

					iterator() = default;

					explicit iterator(
						concat_view* parentPtr,
						bool isInFirst,
						::std::optional<It1_t> iterFirstOptInit,
						::std::optional<It2_t> iterSecondOptInit
					)
						: parent{ parentPtr }
						, inFirst{ isInFirst }
						, iterFirstOpt{ ::std::move(iterFirstOptInit) }
						, iterSecondOpt{ ::std::move(iterSecondOptInit) } {
					}

					reference operator*() const {
						// Разыменование транслируется либо к итератору первой части, либо ко второй.
						if (this->inFirst) {
							return **this->iterFirstOpt;
						}
						else {
							return **this->iterSecondOpt;
						}
					}

					iterator& operator++() {
						// Шаг итератора:
						//   - если мы в первой части — пытаемся сдвинуться;
						//     при достижении конца первой части переключаемся на начало второй.
						//   - если во второй — сдвигаем её итератор.
						if (this->inFirst) {
							auto& it1 = *this->iterFirstOpt;
							++it1;

							if (it1 == ::std::ranges::end(this->parent->viewFirst)) {
								this->inFirst = false;
								this->iterSecondOpt = ::std::ranges::begin(this->parent->viewSecond);
							}
						}
						else {
							auto& it2 = *this->iterSecondOpt;
							++it2;
						}

						return *this;
					}

					iterator operator++(int) {
						auto iteratorCopy = *this;
						++(*this);
						return iteratorCopy;
					}

					// Методная форма сравнения: it == default_sentinel
					bool operator==(::std::default_sentinel_t) const {
						// Конец достигается ТОЛЬКО когда мы уже во второй части и дошли до её end().
						if (this->inFirst) {
							return false;
						}

						return *(this->iterSecondOpt) == ::std::ranges::end(this->parent->viewSecond);
					}

					// Симметричная свободная форма: default_sentinel == it
					friend bool operator==(::std::default_sentinel_t s, const iterator& it) {
						return it == s;
					}

				private:
					concat_view* parent;
					bool inFirst;
					::std::optional<It1_t> iterFirstOpt;
					::std::optional<It2_t> iterSecondOpt;
				};


				iterator begin() {
					auto it1 = ::std::ranges::begin(this->viewFirst);
					auto e1 = ::std::ranges::end(this->viewFirst);

					// Если первая часть пуста — начинаем сразу со второй.
					// Иначе — начинаем с первой.
					if (it1 == e1) {
						auto it2 = ::std::ranges::begin(this->viewSecond);

						return iterator{
							this,
							false,
							::std::nullopt,
							::std::optional{ it2 }
						};
					}
					else {
						return iterator{
							this,
							true,
							::std::optional{ it1 },
							::std::nullopt
						};
					}
				}

				::std::default_sentinel_t end() {
					return ::std::default_sentinel;
				}

			private:
				// Храним оба представления ПО ЗНАЧЕНИЮ — так мы контролируем их lifetime
				// на всём протяжении итерирования результирующего view.
				V1 viewFirst;
				V2 viewSecond;
			};

			//
			// ░ concat_losure
			//
			// Объект-замыкание для пайп-синтаксиса.
			//   v1 | concat(v2)
			// храним второй операнд (как view), а первый придёт в operator|.
			// Гарантируем lifetime второго за счёт хранения внутри closure.
			//
			template <typename R2>
				requires ::std::ranges::viewable_range<R2>
			struct concat_closure {
				ranges::tools::view_of_t<R2> viewSecond;

				template <typename R1>
					requires ::std::ranges::viewable_range<R1>
				friend auto operator|(
					R1&& viewFirst,
					const concat_closure& closure
					) {
					using result_t = concat_view<
						ranges::tools::view_of_t<R1>,
						ranges::tools::view_of_t<R2>
					>;
					return result_t{
						views::tools::as_view(::std::forward<R1>(viewFirst)),
						closure.viewSecond
					};
				}
			};

			//
			// ░ concat_fn
			//
			// Функциональный объект-адаптер:
			//   - унарная форма: concat(r2) -> closure (для пайпа);
			//   - бинарная форма: concat(r1, r2) -> сразу concat_view.
			//
			struct concat_fn {
				template <typename R2>
					requires ::std::ranges::viewable_range<R2>
				auto operator()(
					R2&& r2
					) const {
					return concat_closure<R2>{
						.viewSecond = views::tools::as_view(::std::forward<R2>(r2))
					};
				}

				template <
					typename R1,
					typename R2
				>
					requires HELPERS_NS::meta::concepts::rule<
						HELPERS_NS::meta::concepts::placeholder,
							::std::ranges::viewable_range<R1> and
							::std::ranges::viewable_range<R2>
					>
				auto operator()(
					R1&& r1,
					R2&& r2
					) const {
					using result_t = concat_view<
						ranges::tools::view_of_t<R1>,
						ranges::tools::view_of_t<R2>
					>;
					return result_t{
						views::tools::as_view(::std::forward<R1>(r1)),
						views::tools::as_view(::std::forward<R2>(r2))
					};
				}
			};

			inline constexpr concat_fn concat{};
		}
	}

	namespace views = ranges::views;
}