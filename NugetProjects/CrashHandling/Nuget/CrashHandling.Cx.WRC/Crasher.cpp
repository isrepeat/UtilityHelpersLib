#include "Crasher.h"
#include <Windows.h>
#include <thread>

namespace CrashHandling {
	namespace Cx {
		namespace Test {
			void DoAccessVialation() {
				//int aaa = 9;

				int* ptr = new int{ 1 };
				delete ptr;

				*ptr = 17;

				//__try {
				//	*(int*)0 = 0;
				//}
				//__except (MyCustomExceptionHandler(GetExceptionInformation())) {
				//}

				int bbb = 9;
			}

			Crasher::Crasher() {
			}

			void Crasher::AccessViolation() {
				DoAccessVialation();
			}

			void Crasher::AccessViolationInOtherThread() {
				std::thread([] {
					DoAccessVialation();
					}).detach();

				// WARNING: minidumpWriter cannot be called if AccessViolationInOtherThread() called in UWP UI thread and block it
				//if (th.joinable())
				//	th.join();
			}
		}
	}
}