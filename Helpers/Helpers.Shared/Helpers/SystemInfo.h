#pragma once
#include "common.h"
#if COMPILE_FOR_DESKTOP
#include <string>

// TODO: rewrite as singleton for usage in other singletons (or init other static variables)
namespace HELPERS_NS {
	std::wstring GetUserNameW();
	std::wstring GetComputerNameW();
}
#endif