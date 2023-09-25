#include "Helpers.h"
#include "Backtrace.h"
#include "StackTrace.h"
#include <StacktraceRestorer\StacktraceRestorer.h>

namespace H {
    namespace System {
        std::wstring BuildStacktrace() {
            return BuildStacktrace(std::make_shared<Backtrace>(2));
        }

        std::wstring BuildStacktrace(std::shared_ptr<Backtrace> backtrace) {
            if (!backtrace)
                return L"";

            int counter = 0;
            std::wstring stacktrace;
            auto stackFrames = StacktraceRestorer::BuildStacktrace(backtrace->GetBacktrace());
            for (auto& frame : stackFrames) {
                std::vector<wchar_t> buff(2048, '\0');
                swprintf_s(buff.data(), buff.size(), L"#%d %s:%d  %s \n\n", ++counter, frame.filename.c_str(), frame.lineNumber, frame.symbolName.c_str());

                stacktrace += VecToWStr(buff);
            }

            return stacktrace;
        }

        std::wstring GetBacktraceAsString(int skipFrames) {
            std::wstring backtraceStr;
            auto backtrace = StacktraceRestorer::BackTrace(skipFrames);

            for (auto& [modulename, backtraceFrames] : backtrace) {
                for (auto& backtraceFrame : backtraceFrames) {
                    std::vector<wchar_t> buff(256, '\0');
                    swprintf_s(buff.data(), buff.size(), L"%s 0x%08lx \n", backtraceFrame.moduleName.c_str(), backtraceFrame.RVA);

                    backtraceStr += VecToWStr(buff);
                }
            }

            return backtraceStr;
        }
    }
}