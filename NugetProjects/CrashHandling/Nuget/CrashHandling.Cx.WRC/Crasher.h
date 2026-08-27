#pragma once

namespace CrashHandling {
    namespace Cx {
        namespace Test {
            public ref class Crasher sealed {
            public:
                Crasher();

                void AccessViolation();
                void AccessViolationInOtherThread();
            };
        }
    }
}