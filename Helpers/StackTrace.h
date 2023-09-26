#pragma once
#include <string>
#include <memory>

namespace H {
	namespace System {
		class Backtrace;
		std::wstring BuildStacktrace();
		std::wstring BuildStacktrace(std::shared_ptr<Backtrace> backtrace);
		std::wstring GetBacktraceAsString(int skipFrames = 0);
	}
}