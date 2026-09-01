#pragma once

#include "XamlRuntime/XamlLayout.h"

namespace xaml {
    Element* HitTest(Element& root, float x, float y);
    bool HandleTap(Element& element);
}