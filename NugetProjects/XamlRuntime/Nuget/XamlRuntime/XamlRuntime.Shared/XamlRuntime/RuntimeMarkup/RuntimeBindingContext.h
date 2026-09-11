#pragma once
#include "RuntimeMarkup/RuntimeBindingRegistry.h"
#include "RuntimeMarkup/XamlAst.h"

namespace xaml::runtime {
    struct RuntimeBindingContext {
        std::shared_ptr<RuntimeBindingRegistry> bindings;
        std::string owner;
        std::map<std::string, std::function<std::unique_ptr<Element>(BindingScope&)>> controls;
        std::function<void(Element&)> prepareTree;
        std::function<void()> beforeCommit;
    };
}