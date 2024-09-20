#pragma once
#include "Helpers/common.h"
#include "JsonParser/JsonParser.h"
#include "Helpers/Concepts.h"
#include "Helpers/Logger.h"
#include <fstream>
#include <vector>
#include <string>

namespace HELPERS_NS {
	template <typename JSONObjectT>
#if __cpp_concepts
		requires concepts::Conjunction<
			concepts::HasStaticMethodWithSignature<decltype(&JSONObjectT::AfterLoadHandler), void(*)(const JSONObjectT&)>
		>
#endif
	class JSONLoader {
		JSONLoader() = delete;
		~JSONLoader() = delete;

	private:
		template <typename AppFeaturesImplT>
		friend class AppFeaturesBase;

		// Return 'true' if the 'AfterLoadHandler' was successfuly called:
		// - json file exist and could be parsed success;
		// - json file not exist and could be saved success;
		static bool Load(const std::filesystem::path& filepath) {
			LOG_FUNCTION_ENTER("Load(filepath = {})", filepath.string());
			std::string jsonFilename = filepath.filename().string();

			std::ifstream jsonFileStream(filepath, std::ios::binary);
			if (jsonFileStream.is_open()) {
				std::vector<char> byteArray = std::vector<char>(
					std::istreambuf_iterator<char>(jsonFileStream),
					std::istreambuf_iterator<char>());

				jsonFileStream.close();

				LOG_DEBUG_D("\"{}\" data: \n{}", jsonFilename, HELPERS_NS::VecToStr(byteArray));

				// To avoid merge results after parsing ensure that all JSON objects is empty.
				JSONObjectT jsonObject;
				if (JS::ParseTo(byteArray, jsonObject)) {
					JSONObjectT::AfterLoadHandler(jsonObject);
					return true;
				}
				else {
					LOG_ERROR_D("Cannot parse '{}'", jsonFilename);
					constexpr bool has_LoadErrorHandler = requires {
						JSONObjectT::LoadErrorHandler();
					};
					if constexpr (has_LoadErrorHandler) {
						JSONObjectT::LoadErrorHandler();
					}
				}
			}
			else {
				LOG_WARNING_D("{} not found, create with default json", jsonFilename);
				
				JSONObjectT jsonObject;
				// If JSONObjectT has 'CreateWithDefaultData' static method so create json with 
				// specific default data before save.
				constexpr bool has_CreateWithDefaultData = requires {
					requires concepts::HasStaticMethodWithSignature<decltype(&JSONObjectT::CreateWithDefaultData), JSONObjectT(*)()>;
				};
				if constexpr (has_CreateWithDefaultData) {
					jsonObject = JSONObjectT::CreateWithDefaultData();
				}

				if (Save(filepath, jsonObject)) {
					JSONObjectT::AfterLoadHandler(jsonObject);
					return true;
				}
			}
			return false;
		}

		static bool Save(const std::filesystem::path& filepath, const JSONObjectT& jsonObject) {
			try {
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
			catch (...) {
				LOG_ERROR_D("Cannot save config");
				LogLastError;
				return false;
			}
			return true;
		}
	};
}