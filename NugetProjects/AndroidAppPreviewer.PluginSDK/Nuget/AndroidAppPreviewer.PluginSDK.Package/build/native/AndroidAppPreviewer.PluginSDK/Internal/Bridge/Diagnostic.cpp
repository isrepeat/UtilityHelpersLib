#include "Diagnostic.h"

namespace preview_sdk::bridge {
    std::string& LastError() {
        static thread_local std::string lastError;
        return lastError;
    }
}