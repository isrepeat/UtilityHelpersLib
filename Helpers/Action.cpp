#pragma once
#include "Action.h"

namespace {
	struct CompleteException {};
}

namespace H {
	PerformActionWithAttempts::PerformActionWithAttempts(int attempts, std::function<void()> actionCallback) {
		try {
			while (attempts-- > 0) {
				actionCallback(); // may throw CompleteException
			}
		}
		catch (const CompleteException&) {
			// Action completed
		}
	}

	void PerformActionWithAttempts::Break() {
		throw CompleteException{};
	}
}