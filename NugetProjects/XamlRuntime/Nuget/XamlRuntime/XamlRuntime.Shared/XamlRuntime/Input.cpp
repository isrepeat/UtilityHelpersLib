#include <Helpers.Logging/Logging.h>

#include "Input.h"

namespace xaml::_details {
    bool Contains(const Rect& bounds, float x, float y) {
        return x >= bounds.x && x <= bounds.x + bounds.width
            && y >= bounds.y && y <= bounds.y + bounds.height;
    }

    bool IsInteractive(const Element& element) {
        const ElementType type = element.Type();
        return type == ElementType::border
            || type == ElementType::button
            || type == ElementType::iconButton
            || type == ElementType::toggleSwitch;
    }

    Element* HitTestElement(Element& element, float x, float y) {
        if (element.VisibilityValue() != attr::Visibility::visible
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
    Element* HitTest(Element& root, float x, float y) {
        return _details::HitTestElement(root, x, y);
    }

    bool HandleTap(Element& element) {
        if (element.Type() == ElementType::toggleSwitch) {
            element.SetIsOn(!element.IsOn());
            LOG_DEBUG(
                "XamlRuntime.Input",
                "Toggle tap: element='{}', isOn={}",
                element.Id(),
                element.IsOn());
            return true;
        }
        const bool handled = element.Type() == ElementType::border
            || element.Type() == ElementType::button
            || element.Type() == ElementType::iconButton;
        if (handled) {
            LOG_DEBUG(
                "XamlRuntime.Input",
                "Tap: element='{}'",
                element.Id());
        }
        return handled;
    }
}