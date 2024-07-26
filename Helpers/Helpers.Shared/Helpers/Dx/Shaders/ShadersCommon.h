#pragma once
#include <Helpers/common.h>
#include <Helpers/Helpers.h>

#if COMPILE_FOR_DESKTOP
	const std::filesystem::path g_shaderLoadDir = H::ExePath();
#elif COMPILE_FOR_CX
	// TODO: use predefined macro with root project ns name.
	const std::filesystem::path g_shaderLoadDir = H::ExePath() / L"Helpers_UWP";
#endif