#include "Logger.h"
#include "Helpers.h"
#pragma push_macro("HELPERS_NS")
#define HELPERS_NS NUGET_HELPERS_NS
#pragma pop_macro("HELPERS_NS")


namespace CppFeatures {
	namespace Cx {
		void CppFeatures::Cx::Logger::Init(Platform::String^ logFile, Platform::IBox<InitFlags>^ initFlags) {
			if (initFlags == nullptr) {
				CppFeatures::Logger::Init(logFile->Data(), static_cast<CppFeatures::Logger::InitFlags>(Cx::InitFlags::DefaultFlags));
			}
			else {
				CppFeatures::Logger::Init(logFile->Data(), static_cast<CppFeatures::Logger::InitFlags>(initFlags->Value));
			}
		}

		void Logger::LogDebug(Platform::String^ message, Platform::String^ filename, Platform::String^ memberName, int lineNumber) {
			Logger::Log(
				message,
				filename,
				memberName,
				lineNumber,
				LogPattern::Debug,
				LogLevel::Debug
			);
		}

		void Logger::Log(
			Platform::String^ message,
			Platform::String^ filename,
			Platform::String^ memberName,
			int lineNumber,
			LogPattern pattern,
			LogLevel level
		) {
			CppFeatures::Logger::Log(CppFeatures::Logger::Context{ details::WStrToStr(filename->Data()),  details::WStrToStr(memberName->Data()), lineNumber },
				static_cast<CppFeatures::Logger::Pattern>(pattern),
				static_cast<CppFeatures::Logger::Level>(level),
				message->Data());
		}
	}
}
