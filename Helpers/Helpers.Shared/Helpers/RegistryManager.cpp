#include "RegistryManager.h"
#include "HString.h"
#include <vector>
#include "Logger.h"
#include "Helpers.h"

namespace {
	HKEY ConvertToHKEY(HKey hKey) {
		switch (hKey) {
		case HKey::ClassesRoot:
			return HKEY_CLASSES_ROOT;
		case HKey::CurrentUser:
			return HKEY_CURRENT_USER;
		case HKey::LocalMachine:
			return HKEY_LOCAL_MACHINE;
		case HKey::Users:
			return HKEY_USERS;
		}
		return nullptr;
	}
}

namespace HELPERS_NS {
	bool RegistryManager::HasRegValue(HKey hKey, const std::string& path, const std::string& keyName) {
		LOG_FUNCTION_ENTER("HasRegValue({}, {})", path, keyName);

		std::vector<char> buff(255);
		DWORD sz = buff.size();

		auto status = RegGetValueA(ConvertToHKEY(hKey), path.c_str(), keyName.c_str(), RRF_RT_ANY | RRF_SUBKEY_WOW6464KEY, NULL, buff.data(), &sz);
		if (status != S_OK) {
			LOG_DEBUG_D("Not found reg value");
			return false;
		}

		return true;
	}

	std::string RegistryManager::GetRegValue(HKey hKey, const std::string& path, const std::string& keyName) {
		LOG_FUNCTION_ENTER("GetRegValue({}, {})", path, keyName);

		std::vector<char> buff(255);
		DWORD sz = buff.size();

		auto status = RegGetValueA(ConvertToHKEY(hKey), path.c_str(), keyName.c_str(), RRF_RT_ANY | RRF_SUBKEY_WOW6464KEY, NULL, buff.data(), &sz);
		if (status != S_OK) {
			LOG_WARNING_D("Can't get reg value. Status = {}", status);
			return "";
		}
		
		return HELPERS_NS::VecToStr(buff);
	}

	// TODO: rewrite with wstring (be careful with double length of character)
	void RegistryManager::SetRegValue(HKey hKey, const std::string& path, const std::string& keyName, const std::string& value) {
		LOG_FUNCTION_ENTER("SetRegValue({}, {})", path, keyName);

		auto status = RegSetKeyValueA(ConvertToHKEY(hKey), path.c_str(), keyName.c_str(), REG_SZ, value.c_str(), value.size());
		if (status != S_OK) {
			LOG_WARNING_D("Can't set reg value. Status = {}", status);
		}
	}
}