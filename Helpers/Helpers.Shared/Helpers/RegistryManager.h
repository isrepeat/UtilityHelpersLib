#pragma once
#include "common.h"
#if COMPILE_FOR_DESKTOP
#include <string>
#include <filesystem>

namespace HELPERS_NS {
    enum class HKey {
        ClassesRoot,
        CurrentUser,
        LocalMachine,
        Users,
        CurrentConfig,
    };

    enum class RegAction {
        Get,
        Add,
    };

    struct RegCommand {
        RegAction regAction = RegAction::Add;
        HKey hKey = HKey::CurrentUser;
        std::filesystem::path path;
        std::string keyName;
        std::string value;
    };

    class RegistryManager {
    public:
        static bool HasRegValue(HKey hKey, const std::filesystem::path& path, const std::string& keyName);
        static std::string GetRegValue(HKey hKey, const std::filesystem::path& path, const std::string& keyName);
        static void SetRegValue(HKey hKey, const std::filesystem::path& path, const std::string& keyName, const std::string& value);
        
        static bool HasRegValue(RegCommand regCommand);
        static std::string GetRegValue(RegCommand regCommand);
        static void SetRegValue(RegCommand regCommand);
        static void SetRegValuesAdmin(std::vector<RegCommand> regCommands);

    private:
        static std::string MakeRegShellCommand(RegCommand regCommand);
        static void ExecuteRegCommandWithShellAdmin(std::string regShellCommand);
    };
}
#endif