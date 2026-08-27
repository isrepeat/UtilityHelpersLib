#pragma once
#include "../CppFeatures.Shared/LoggerNative.h"
#include "NamespacesAliases.h"

namespace CppFeatures {
	namespace Cx {
		public enum class InitFlags {
			LogInitFlags_Enum
		};

		public enum class LogPattern {
			LogPattern_Enum
		};

		public enum class LogLevel {
			LogLevel_Enum
		};

		public ref class Logger sealed {
		public:
			static void Init(Platform::String^ logFile, Platform::IBox<InitFlags>^ initFlags);
			static void Log(
				Platform::String^ message,
				Platform::String^ filename,
				Platform::String^ memberName,
				int lineNumber,
				LogPattern pattern,
				LogLevel level
			);
			static void LogDebug(Platform::String^ message, Platform::String^ filename, Platform::String^ memberName, int lineNumber);
		};
	}
}
