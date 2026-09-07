#include <Helpers.Logging/Logging.h>

#include "Input.h"

namespace xaml::_details {
    bool Contains(const Rect& bounds, float x, float y) {
        return x >= bounds.x && x <= bounds.x + bounds.width
            && y >= bounds.y && y <= bounds.y + bounds.height;
    }

    bool HasIntrinsicInteraction(const Element& element) {
        switch (element.Type()) {
        case ElementType::button:
        case ElementType::iconButton:
        case ElementType::toggleSwitch:
            return true;
        default:
            return false;
        }
    }

    Element* HitTestElement(Element& element, float x, float y) {
        if (!element.CanReceiveInput()
            || !element.IsEnabled()
            || !Contains(element.ClipBounds(), x, y)) {
            return nullptr;
        }

        std::vector<std::unique_ptr<Element>>& children = element.Children();
        for (auto child = children.rbegin(); child != children.rend(); ++child) {
            if (Element* const hit = HitTestElement(**child, x, y)) {
                return hit;
            }
        }
        return IsInteractive(element) && Contains(element.Bounds(), x, y) ? &element : nullptr;
    }
}

namespace xaml {
    bool IsInteractive(const Element& element) {
        return _details::HasIntrinsicInteraction(element) || element.HasCommand();
    }

    Element* HitTest(Element& root, float x, float y) {
        return _details::HitTestElement(root, x, y);
    }

    bool HandleTap(Element& element) {
        if (!IsInteractive(element)) {
            return false;
        }
        if (element.Type() == ElementType::toggleSwitch) {
            element.SetIsOn(!element.IsOn());
            LOG_DEBUG(
                "XamlRuntime.Input",
                "Toggle tap: element='{}', isOn={}",
                element.Id(),
                element.IsOn());
            return true;
        }
        if (element.HasCommand()) {
            LOG_DEBUG(
                "XamlRuntime.Input",
                "Tap: element='{}'",
                element.Id());
        }
        return true;
    }
}