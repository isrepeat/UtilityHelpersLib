#include "Diagnostic.h"

namespace {{application}}::preview::bridge {
    std::string& LastError() {
        static thread_local std::string lastError;
        return lastError;
    }
}