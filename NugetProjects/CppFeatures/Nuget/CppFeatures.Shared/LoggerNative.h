#pragma once
#include "RootHeader.h"

// Ensure that flags are the same with original
#define LogInitFlags_Enum                                                    \
    None = 0x00,                                                             \
    Truncate = 0x01,                                                         \
    AppendNewSessionMsg = 0x02,                                              \
    CreateInPackageFolder = 0x04,                                            \
    EnableLogToStdout = 0x08,                                                \
    RedirectRawTimeLogToStdout = 0x10,                                       \
    DisableEOLforRawLogger = 0x20,                                           \
    CreateInExeFolderForDesktop = 0x40,                                      \
    DefaultFlags = AppendNewSessionMsg,                                      \
    CreateInAppFolder = CreateInPackageFolder | CreateInExeFolderForDesktop

#define LogPattern_Enum						\
    Default,								\
    Raw,									\
    Time,									\
    Func,                                   \
    Extend,                                 \
    Debug,                                  \
    DebugFn,                                \

#define LogLevel_Enum						\
   Trace = 0,								\
   Debug,									\
   Info,									\
   Warning,									\
   Error,									\
   Critical,								\
   Off

namespace NUGET_HELPERS_NS {
	template<typename EnumMsg, typename T>
	class Channel;

	template<typename R, typename... Ts>
	class Callback;
}

namespace CppFeatures {
	using string_t = std::wstring;

	class CPPFEATURES_API Logger {
	public:
		struct Context {
			const std::string filename = nullptr;
			const std::string function = nullptr;
			const int line = 0;
		};

		enum class InitFlags {
			LogInitFlags_Enum
		};
		enum class Pattern {
			LogPattern_Enum
		};
		enum class Level {
			LogLevel_Enum,
			N_levels
		};

		static void Init(const std::wstring& logFilePath, InitFlags initFlags);
		static void Log(Context context, Pattern pattern, Level level, const string_t& format);

	private:
		Logger() = delete; // not used because desktop logger-singleton is used inside
	};
}