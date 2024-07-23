#pragma once
#include "common.h"

namespace HELPERS_NS {
	struct EmptyMutex {
		void lock() {}
		void unlock() {}
	};
}