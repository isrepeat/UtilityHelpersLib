#pragma once
#include "common.h"
#include <string>

namespace HELPERS_NS {
    class PackageProvider {
    public:
        static bool IsRunningUnderPackage();
        static std::wstring GetPackageVersion();
        static std::wstring GetPackageFamilyName();
        static std::wstring GetApplicationUserModelId();
    };
}