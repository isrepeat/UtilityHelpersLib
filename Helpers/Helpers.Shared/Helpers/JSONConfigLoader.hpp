#pragma once
#include "Helpers/common.h"
#include "JsonParser/JsonParser.h"
#include "Helpers/Logger.h"
#include <fstream>
#include <vector>
#include <string>

namespace HELPERS_NS {
	template <typename JSONObjectT, typename LoadFromT>
	class JSONConfigLoader {
		JSONConfigLoader() = delete;
		~JSONConfigLoader() = delete;

	private:
		friend LoadFromT;

		static bool Load(const std::filesystem::path& filepath) {
			LOG_FUNCTION_ENTER("Load(filepath = {})", filepath.string());
			std::string configName = filepath.filename().string();

			std::ifstream jsonConfigFile(filepath, std::ios::binary);
			if (jsonConfigFile.is_open()) {
				std::vector<char> byteArray = std::vector<char>(
					std::istreambuf_iterator<char>(jsonConfigFile),
					std::istreambuf_iterator<char>());

				jsonConfigFile.close();

				LOG_DEBUG_D("\"{}\" data: \n{}", configName, H::VecToStr(byteArray));

				JSONObjectT jsonObject;
				if (JS::ParseTo(byteArray, jsonObject)) {
					JSONObjectT::AfterLoadHandler(jsonObject);
				}
				else {
					LOG_ERROR_D("Cannot parse '{}'", configName);
					return false;
				}
			}
			else {
				LOG_WARNING_D("{} not found, create config with default json", configName);
				Save(filepath, JSONObjectT{});
			}

			return true;
		}
		static void Save(const std::filesystem::path& filepath, const JSONObjectT& jsonObject) {
			LOG_FUNCTION_ENTER("Save(filepath, jsonObject)");
			// You can use here for example BeforeSaveHandler(...) in future

			auto serializedStruct = JS::serializeStruct(jsonObject);
			LOG_DEBUG_D("Serialized jsonObject data: \n{}", serializedStruct);

			bool result = std::filesystem::create_directory(filepath.parent_path());

			std::ofstream outFile;
			outFile.open(filepath);
			outFile.write(serializedStruct.data(), serializedStruct.size());
			outFile.close();
		}
	};
}