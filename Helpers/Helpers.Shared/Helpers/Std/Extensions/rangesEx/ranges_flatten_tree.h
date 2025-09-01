#pragma once
#include "Helpers/common.h"
#include "Helpers/Meta/Concepts.h"
#include "Tools.h"

#include <type_traits>
#include <algorithm>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>
#include <ranges>

namespace STD_EXT_NS {
	namespace ranges {
		namespace views {
			//
			// ░ flatten_tree
			// ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
			//
			// Идея:

			namespace details {
				namespace concepts {
					//
					// TGetChildrenFn соответствует узлу TNode
					//
					template <
						typename TGetChildrenFn,
						typename TNode
					>
					concept children_provider_for = requires(
						TGetChildrenFn getChildrenFn,
						TNode node
						) {
							{ getChildrenFn(node) } -> ::std::ranges::input_range;
							requires
							::std::same_as<
								::std::ranges::range_value_t<::std::invoke_result_t<TGetChildrenFn, TNode>>,
								TNode
							>;
					};
				}
			}

			
			//
			// ░ flatten_tree_view
			//
			template <
				typename TView,
				typename TGetChildrenFn
			>
			__requires requires { requires
				::std::ranges::view<TView>;
			}
			class flatten_tree_view : public ::std::ranges::view_interface<flatten_tree_view<TView, TGetChildrenFn>> {
			public:
				using Node_t = ::std::ranges::range_value_t<TView>;
				
				class iterator {
				public:
					// Соглашения итераторов для STL
					using iterator_concept = ::std::input_iterator_tag;
					using iterator_category = ::std::input_iterator_tag;
					using difference_type = ::std::ptrdiff_t;
					using value_type = Node_t;
					using reference = Node_t;

				public:
					iterator() = default;

					explicit iterator(
						const TGetChildrenFn* getChildrenFnPtr,
						::std::vector<Node_t>&& roots
					)
						: getChildrenFnPtr{ getChildrenFnPtr }
						, stack{ ::std::move(roots) } {
						// stack изначально содержит только корни, причем в прямом порядке.
						// Для правильного обхода слева-напрво разворачиваем корни.
						::std::reverse(this->stack.begin(), this->stack.end());
						this->advance_to_next();
					}

					reference operator*() const {
						return this->currentNode;
					}

					const Node_t* operator->() const {
						return ::std::addressof(this->currentNode);
					}

					iterator& operator++() {
						this->advance_to_next();
						return *this;
					}

					void operator++(int) {
						++(*this);
					}

					bool operator==(::std::default_sentinel_t) const {
						return this->atEnd;
					}

					friend bool operator==(::std::default_sentinel_t s, const iterator& it) {
						return it == s;
					}

				private:
					void advance_to_next() {
						// Реализует обход дерева слева-направо.
						// К примру есть дерево:
						// Root
						//  ├─ A
						//  │  ├─ A1
						//  │  └─ A2
						//  └─ B
						//     └─ B1
						// 
						// тогда результат обхода будет: [Root, A, A1, A2, B, B1, B2].

						if (this->stack.empty()) {
							this->atEnd = true;
							return;
						}

						this->currentNode = ::std::move(this->stack.back());
						this->stack.pop_back();

						auto currentNodeСhildrenView = (*this->getChildrenFnPtr)(this->currentNode);

						// Собираем детей «слева направо»:
						::std::vector<Node_t> currentNodeСhildren;
						for (auto&& child : currentNodeСhildrenView) {
							if (static_cast<bool>(child)) {
								currentNodeСhildren.push_back(child);
							}
						}

						// ...и затем кладём их в стек в ОБРАТНОМ порядке (right => left),
						// чтобы при LIFO-извлечении порядок обхода стал left => right.
						// 
						// К примеру если сейчас stack = [B, A*, Root*] == [B]
						// то после цикла станет stack = [B, A*, Root*, A2, A1] == [B, A2, A1]
						// (* - означает что элемент пройден и удален из стека)
						for (auto it = currentNodeСhildren.rbegin(); it != currentNodeСhildren.rend(); ++it) {
							this->stack.push_back(::std::move(*it));
						}
					}

				private:
					const TGetChildrenFn* getChildrenFnPtr{ nullptr };
					::std::vector<Node_t> stack; // LIFO-стек для обхода слева-направо
					Node_t currentNode;
					bool atEnd = false;
				}; // class iterator


				flatten_tree_view(
					TView rootsView,
					const TGetChildrenFn& getChildrenFn
				)
					: rootsView{ ::std::move(rootsView) }
					, getChildrenFn{ getChildrenFn } {
					static_assert(
						details::concepts::children_provider_for<TGetChildrenFn, Node_t>,
						"TGetChildrenFn must return an input_range with range_value_t == Node_t"
						);
				}

				auto begin() {
					::std::vector<Node_t> roots;

					// Собираем корни «слева направо»:
					for (auto&& r : this->rootsView) {
						if (static_cast<bool>(r)) {
							roots.push_back(r);
						}
					}

					return iterator{
						::std::addressof(this->getChildrenFn),
						::std::move(roots)
					};
				}

				auto end() {
					return ::std::default_sentinel;
				}

			private:
				TView rootsView;
				TGetChildrenFn getChildrenFn;
			};


			//
			// ░ flatten_tree_fn
			//
			template <typename TGetChildrenFn>
			struct flatten_tree_fn {
				TGetChildrenFn getChildrenFn;

				template <typename TRange>
				__requires requires { requires
					::std::ranges::viewable_range<TRange>;
				}
				auto operator()(
					TRange&& rootsViewable
					) const {
					using TView = ranges::tools::view_of_t<TRange>;
					using Node_t = ::std::ranges::range_value_t<TView>;

					static_assert(
						details::concepts::children_provider_for<TGetChildrenFn, Node_t>,
						"get_children(node) must return an input_range with range_value_t == Node_t"
						);

					return flatten_tree_view<TView, TGetChildrenFn>{
						views::tools::as_view(::std::forward<TRange>(rootsViewable)), this->getChildrenFn
					};
				}
			};


			//
			// ░ flatten_tree_closure
			//
			template <typename TGetChildrenFn>
			struct flatten_tree_closure {
			private:
				TGetChildrenFn getChildrenFn;

			public:
				explicit flatten_tree_closure(TGetChildrenFn getChildrenFn)
					: getChildrenFn{ ::std::move(getChildrenFn) } {
				}

				template <typename TRange>
				__requires requires { requires
					::std::ranges::viewable_range<TRange>;
				}
				friend auto operator|(
					TRange&& roots,
					const flatten_tree_closure& self
					) {
					return flatten_tree_fn<TGetChildrenFn>{ self.getChildrenFn }(::std::forward<TRange>(roots));
				}
			};


			template <typename TGetChildrenFn>
			inline flatten_tree_closure<TGetChildrenFn> flatten_tree(TGetChildrenFn getChildrenFn) {
				return flatten_tree_closure<TGetChildrenFn>{ ::std::move(getChildrenFn) };
			}
		}
	}

	namespace views = ranges::views;
}


//
// ░ Export concepts
// ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 
//
namespace HELPERS_NS {
	namespace meta {
		namespace concepts {
			// ...
		}
	}
}