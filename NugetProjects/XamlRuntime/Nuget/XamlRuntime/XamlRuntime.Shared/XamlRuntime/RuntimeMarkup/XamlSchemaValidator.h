#pragma once
#include "RuntimeMarkup/XamlAst.h"

#include <set>

namespace xaml::runtime {
    class XamlSchemaValidator final {
    public:
        void Validate(const XamlElementNode& root, const std::set<std::string>& controls = {});
    };
}