#pragma once
#include <Helpers/common.h>
#include <Helpers/Helpers.h>

#if COMPILE_FOR_DESKTOP
	const std::filesystem::path g_shaderLoadDir = H::ExePath();
#elif COMPILE_FOR_CX
	const std::filesystem::path g_shaderLoadDir = H::ExePath() / L"" PP_STRINGIFY(MSBuildProject__RootNamespace);
#endif	