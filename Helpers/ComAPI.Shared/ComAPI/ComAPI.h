#pragma once
#include <Windows.h>
#include <functional>
#include <filesystem>
#include <string>
#pragma comment(lib, "RuntimeObject.lib")

//extern "C" {
	namespace ComApi {
		std::filesystem::path GetPackageFolder(); // Absolute path to the App package LocalState folder
		std::wstring WindowsVersion();
	}
//}