#pragma once
#include <functional>

namespace H {
	class PerformActionWithAttempts {
	public:
		PerformActionWithAttempts(int attempts, std::function<void()> actionCallback);
		~PerformActionWithAttempts() = default;

		static void Break(); // throw CompleteException
	};

// use with H::
#define BreakAction  PerformActionWithAttempts::Break()
}