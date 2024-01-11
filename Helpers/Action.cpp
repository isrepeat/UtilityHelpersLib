#pragma once
#include "Action.h"
#include "Logger.h"

namespace {
	struct CompleteException {};
}

namespace HELPERS_NS {
	PerformActionWithAttempts::PerformActionWithAttempts(int attempts, std::function<void()> actionCallback) {
		try {
			while (attempts-- > 0) {
				LOG_DEBUG_D("PerformAction, attempts left = {}", attempts);
				actionCallback(); // may throw CompleteException
			}
		}
		catch (const CompleteException&) {
			LOG_DEBUG_D("Action completed");
		}
	}

	void PerformActionWithAttempts::Break() {
		throw CompleteException{};
	}
}