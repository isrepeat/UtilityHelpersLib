#pragma once
#include <string>

// TODO: rewrite as singleton for usage in other singletons (or init other static variables)
namespace H {
	std::wstring GetUserNameW();
	std::wstring GetComputerNameW();
}