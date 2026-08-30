#include "xaml_runtime/XamlLayout.h"

#include <algorithm>

namespace mobileclock::ui {
    namespace {
        Size measure(Element& element) {
            // Первый проход вычисляет требуемый размер снизу вверх. Точная
            // метрика шрифта появится позже; пока ширина текста оценивается.
            if (element.type() == ElementType::textBlock || element.type() == ElementType::button) {
                Size result{
                    std::max(1.0f, static_cast<float>(element.text().size()) * element.fontSize() * 0.55f),
                    element.fontSize() * 1.25f,
                };
                if (element.type() == ElementType::button) {
                    result.width += 48.0f;
                    result.height += 24.0f;
                }
                element.setDesiredSize(result);
                return result;
            }

            Size result{};
            for (const auto& child : element.children()) {
                const Size childSize = measure(*child);
                if (element.orientation() == Orientation::vertical) {
                    result.width = std::max(result.width, childSize.width);
                    result.height += childSize.height;
                } else {
                    result.width += childSize.width;
                    result.height = std::max(result.height, childSize.height);
                }
            }
            if (!element.children().empty()) {
                // Spacing существует только между соседями, а не после
                // последнего дочернего элемента.
                result.height += element.orientation() == Orientation::vertical
                    ? element.spacing() * static_cast<float>(element.children().size() - 1)
                    : 0.0f;
                result.width += element.orientation() == Orientation::horizontal
                    ? element.spacing() * static_cast<float>(element.children().size() - 1)
                    : 0.0f;
            }
            element.setDesiredSize(result);
            return result;
        }

        void arrange(Element& element, Rect bounds) {
            // Второй проход выдаёт каждому элементу конечный прямоугольник
            // сверху вниз. StackPanel центрирует детей по поперечной оси.
            element.setBounds(bounds);
            float cursor = element.orientation() == Orientation::vertical ? bounds.y : bounds.x;
            for (const auto& child : element.children()) {
                const Size size = child->desiredSize();
                Rect childBounds;
                if (element.orientation() == Orientation::vertical) {
                    childBounds = {bounds.x + (bounds.width - size.width) / 2.0f, cursor, size.width, size.height};
                    cursor += size.height + element.spacing();
                } else {
                    childBounds = {cursor, bounds.y + (bounds.height - size.height) / 2.0f, size.width, size.height};
                    cursor += size.width + element.spacing();
                }
                arrange(*child, childBounds);
            }
        }
    }

    void layout(Element& root, Size availableSize) {
        // Разделение measure/arrange позволяет заменить или расширить layout
        // контейнеры, не меняя контракт визуального дерева.
        const Size desired = measure(root);
        arrange(root, {
            (availableSize.width - desired.width) / 2.0f,
            (availableSize.height - desired.height) / 2.0f,
            desired.width,
            desired.height,
        });
    }
}