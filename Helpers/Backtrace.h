#pragma once
#include <StacktraceRestorer\StacktraceRestorer.h>

namespace H {
	namespace System {
		class Backtrace {
		public:
			Backtrace(int skipFrames = 0)
				: backtrace{ StacktraceRestorer::BackTrace(skipFrames) }
			{}

			StacktraceRestorer::Backtrace GetBacktrace() const {
				return backtrace;
			}

		private:
			StacktraceRestorer::Backtrace backtrace;
		};
	}
}