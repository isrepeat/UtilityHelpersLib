#pragma once
#include "common.h"
#include <functional>

namespace HELPERS_NS {
	// PerformActionWithAttempts also handling "std" | "any" exceptions
	class PerformActionWithAttempts {
	public:
		PerformActionWithAttempts(int attempts, std::function<void()> actionCallback);
		~PerformActionWithAttempts() = default;

		static void Break(); // throw CompleteException
	};

// use with HELPERS_NS::
#define BreakAction  PerformActionWithAttempts::Break()
}