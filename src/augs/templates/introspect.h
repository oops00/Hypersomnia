#pragma once
#include <type_traits>

#include "generated/introspectors.h"

#include "augs/templates/container_templates.h"
#include "augs/templates/introspection_traits.h"
#include "augs/templates/recursive.h"
#include "augs/templates/is_optional.h"

namespace augs {
	/*
		Simple introspection with just one level of depth.
		Will invoke a callback upon every top-level field of a struct.
	*/

	template <
		class F, 
		class Instance, 
		class... Instances
	>
	void introspect(
		F&& callback,
		Instance& t,
		Instances&... tn
	) {
		using T = std::remove_reference_t<Instance>;
		static_assert(has_introspect_v<T>, "Recursion requested on type(s) without introspectors!");

		introspection_access::introspect_body(
			static_cast<std::decay_t<Instance>*>(nullptr), 
			std::forward<F>(callback), t, tn...
		);
	}
	/*
		Simple introspection with just one level of depth.
		Will invoke a callback upon every top-level field of a struct.
		Will not invoke the callback if all the types are introspective leaves.
		If a type is not an introspective leaf, but does not possess an introspector, a compilation error will be raised.
	*/

	template <
		class F, 
		class Instance, 
		class... Instances
	>
	void introspect_if_not_leaf(
		F&& callback,
		Instance& t,
		Instances&... tn
	) {
		using T = std::remove_reference_t<Instance>;

		if constexpr(!is_introspective_leaf_v<T>) {
			introspection_access::introspect_body(
				static_cast<std::decay_t<Instance>*>(nullptr),
				std::forward<F>(callback), t, tn...
			);
		}
	}

	template <class A, class B>
	bool equal_by_introspection(
		const A& a,
		const B& b
	) {
		static_assert(has_introspect_v<A> && has_introspect_v<B>, "Comparison requested on type(s) without introspectors!");

		bool are_equal = true;

		introspect(
			recursive([&are_equal](
				auto&& self,
				const auto label,
				const auto& aa, 
				const auto& bb
			) {
				using AA = std::decay_t<decltype(aa)>;
				using BB = std::decay_t<decltype(bb)>;

				if constexpr(is_optional_v<AA>) {
					static_assert(is_optional_v<BB>);

					if (!aa && !bb) {
						are_equal = are_equal && true;
					}
					else if (aa && bb) {
						self(self, "", *aa, *bb);
					}
					else {
						are_equal = are_equal && false;
					}
				}
				else if constexpr(is_comparable_v<AA, BB>) {
					are_equal = are_equal && aa == bb;
				}
				else {
					introspect(recursive(self), aa, bb);
				}
			}),
			a,
			b
		);
		
		return are_equal;
	}

	template <class T>
	void recursive_clear(T& object) {
		introspect(recursive([](auto&& self, auto, auto& field) {
			using Field = std::decay_t<decltype(field)>;

			if constexpr(can_clear_v<Field>) {
				field.clear();
			}
			else {
				introspect_if_not_leaf(recursive(self), field);
			}
		}), object);
	}
}