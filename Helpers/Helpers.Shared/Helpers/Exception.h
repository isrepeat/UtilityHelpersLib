#pragma once
#include "common.h"
#include <stdexcept>

namespace HELPERS_NS {
    class NugetNotFoundException : public std::logic_error {
    public:
        NugetNotFoundException() 
            : std::logic_error("No nuget for this operation")
        {
        }
    };
}