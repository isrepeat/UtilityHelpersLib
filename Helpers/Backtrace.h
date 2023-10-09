#pragma once
#ifdef CRASH_HANDLING_NUGET
#include <CrashHandling/CrashHandling.h>

namespace H {
	namespace System {
		class Backtrace {
		public:
			Backtrace(int skipFrames = 0)
				: backtrace{ CrashHandling::GetBacktrace(skipFrames) }
			{}

			CrashHandling::Backtrace GetBacktrace() const {
				return backtrace;
			}

			std::wstring GetBacktraceStr() const {
				return CrashHandling::BacktraceToString(backtrace);
			}

		private:
			CrashHandling::Backtrace backtrace;
		};
	}
}
#endif // v