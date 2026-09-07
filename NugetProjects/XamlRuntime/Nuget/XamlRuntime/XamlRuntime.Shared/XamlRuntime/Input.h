#pragma once

#include "XamlRuntime/XamlLayout.h"

namespace xaml {
    bool IsInteractive(const Element& element);
    Element* HitTest(Element& root, float x, float y);
    bool HandleTap(Element& element);
}