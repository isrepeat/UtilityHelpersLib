#include "RegistryManager.h"
#if COMPILE_FOR_DESKTOP
#include <MagicEnum/MagicEnum.h>
#include "Helpers.h"
#include "Logger.h"

#include <vector>
#include <format>
#include <ranges>

namespace HELPERS_NS {
	namespace Tools {
		HKEY ConvertHKeyToWin32HKEY(HKey hKey) {
			switch (hKey) {
			case HKey::ClassesRoot:
				return HKEY_CLASSES_ROOT;
			case HKey::CurrentUser:
				return HKEY_CURRENT_USER;
			case HKey::LocalMachine:
				return HKEY_LOCAL_MACHINE;
			case HKey::Users:
				return HKEY_USERS;
			case HKey::CurrentConfig:
				return HKEY_CURRENT_CONFIG;
			}
			return nullptr;
		}

		std::string ConvertHKeyToCmd(HKey hKey) {
			switch (hKey) {
			case HKey::ClassesRoot:
				return "HKCR";
			case HKey::CurrentUser:
				return "HKCU";
			case HKey::LocalMachine:
				return "HKLM";
			case HKey::Users:
				return "HKU";
			case HKey::CurrentConfig:
				return "HKCC";
			}
			return "";
		}

		std::string ConvertRegActionToCmd(RegAction regAction) {
			switch (regAction) {
			case RegAction::Get:
				return "GET";
			case RegAction::Add:
				return "ADD";
			}
			return "";
		}
	}

	bool RegistryManager::HasRegValue(HKey hKey, const std::filesystem::path& path, const std::string& keyName) {
		LOG_FUNCTION_ENTER("HasRegValue(hKey = {}, path = {}, keyName = {})", MagicEnum::ToString(hKey), path, keyName);
		return RegistryManager::HasRegValue({ RegAction::Get, hKey, path, keyName });
	}

	std::string RegistryManager::GetRegValue(HKey hKey, const std::filesystem::path& path, const std::string& keyName) {
		LOG_FUNCTION_ENTER("GetRegValue(hKey = {}, path = {}, keyName = {})", MagicEnum::ToString(hKey), path, keyName);
		return RegistryManager::GetRegValue({ RegAction::Get, hKey, path, keyName });
	}

	// TODO: rewrite with wstring (be careful with double length of character)
	void RegistryManager::SetRegValue(HKey hKey, const std::filesystem::path& path, const std::string& keyName, const std::string& value) {
		LOG_FUNCTION_ENTER("SetRegValue(hKey = {}, path = {}, keyName = {}, value = {})", MagicEnum::ToString(hKey), path, keyName, value);
		RegistryManager::SetRegValue({ RegAction::Add, hKey, path, keyName, value });
	}



	std::string RegistryManager::GetRegValue(RegCommand regCommand)	{
		auto& [regAction, hKey, path, keyName, value] = regCommand;
		LOG_FUNCTION_ENTER("GetRegValue([hKey = {}, path = {}, keyName = {}, value = {}]", MagicEnum::ToString(regAction), MagicEnum::ToString(hKey), path, keyName, value);
		HRESULT hr = S_OK;

		std::vector<char> buffer(255);
		DWORD dataSize = buffer.size();

		do {
			auto status = RegGetValueA(
				Tools::ConvertHKeyToWin32HKEY(hKey),
				path.string().c_str(),
				keyName.c_str(),
				RRF_RT_ANY | RRF_ZEROONFAILURE | RRF_SUBKEY_WOW6464KEY,
				NULL,
				buffer.data(),
				&dataSize
			);
			hr = HRESULT_FROM_WIN32(status);

			if (hr == ERROR_MORE_DATA) {
				buffer.resize(dataSize);
			}
		} while (hr == ERROR_MORE_DATA);

		if (FAILED(hr)) {
			LOG_FAILED(hr);
			LOG_DEBUG_D("Not found reg value");
			return "";
		}
		return H::VecToStr(buffer);
	}

	bool RegistryManager::HasRegValue(RegCommand regCommand) {
		LOG_FUNCTION_ENTER("HasRegValue(regCommand)");
		if (RegistryManager::GetRegValue(regCommand).empty()) {
			return false;
		}
		return true;
	}

	void RegistryManager::SetRegValue(RegCommand regCommand) {
		auto& [regAction, hKey, path, keyName, value] = regCommand;
		LOG_FUNCTION_ENTER("SetRegValue([hKey = {}, path = {}, keyName = {}, value = {}]", MagicEnum::ToString(regAction), MagicEnum::ToString(hKey), path, keyName, value);
		HRESULT hr = S_OK;

		auto status = RegSetKeyValueA(Tools::ConvertHKeyToWin32HKEY(hKey), path.string().c_str(), keyName.c_str(), REG_SZ, value.c_str(), value.size());
		hr = HRESULT_FROM_WIN32(status);
		if (FAILED(hr)) {
			LOG_FAILED(hr);
			LOG_WARNING_D("Can't set reg value. Status = {}.", status);
		}
	}

	void RegistryManager::SetRegValuesAdmin(std::vector<RegCommand> regCommands) {
		LOG_FUNCTION_ENTER("SetRegValueAdmin(regCommands)");
		if (regCommands.empty()) {
			LOG_WARNING_D("skip settings reg values because 'regCommands' is empty");
			return;
		}

		std::string regShellCommand = std::format("/c ");
		regShellCommand += RegistryManager::MakeRegShellCommand(regCommands.at(0));

		for (auto& regCommand : std::ranges::drop_view{ regCommands, 1 }) {
			regShellCommand += "&";
			regShellCommand += RegistryManager::MakeRegShellCommand(regCommand);
		}
		RegistryManager::ExecuteRegCommandWithShellAdmin(regShellCommand);
	}



	std::string RegistryManager::MakeRegShellCommand(RegCommand regCommand) {
		auto& [regAction, hKey, path, keyName, value] = regCommand;
		LOG_FUNCTION_ENTER("MakeRegShellCommand([hKey = {}, path = {}, keyName = {}, value = {}]", MagicEnum::ToString(regAction), MagicEnum::ToString(hKey), path, keyName, value);

		std::string regShellCommand = std::format(" REG {} \"{}\\{}\" /f "
			, Tools::ConvertRegActionToCmd(regAction)
			, Tools::ConvertHKeyToCmd(hKey)
			, path.string()
		);
		if (!value.empty()) {
			regShellCommand += std::format("/v \"{}\" /t REG_SZ /d \"{}\" "
				, keyName
				, value
			);
		}
		return regShellCommand;
	}

	void RegistryManager::ExecuteRegCommandWithShellAdmin(std::string regShellCommand) {
		LOG_FUNCTION_ENTER("ExecuteRegCommandWithShellAdmin(regShellCommand):\n{}\n", regShellCommand);
		DWORD exitCode;
		H::ExecuteCommandLineA(regShellCommand.c_str(), true, SW_HIDE, &exitCode);
		if (exitCode != 0) {
			LOG_ERROR_D("Execute shell command failed:\n"
				" command: \"{}\"\n"
				" exitCode = {}\n"
				, regShellCommand
				, exitCode
			);
		}
	}
}
#endif