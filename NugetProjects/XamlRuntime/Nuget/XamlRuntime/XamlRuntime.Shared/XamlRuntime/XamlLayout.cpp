#include "XamlRuntime/XamlLayout.h"

#include <algorithm>

namespace mobileclock::ui {
    namespace {
        Size measure(Element& element) {
            // Первый проход вычисляет требуемый размер снизу вверх. Точная
            // метрика шрифта появится позже; пока ширина текста оценивается.
            if (element.Type() == ElementType::textBlock || element.Type() == ElementType::button) {
                Size result{
                    std::max(1.0f, static_cast<float>(element.Text().size()) * element.FontSize() * 0.55f),
                    element.FontSize() * 1.25f,
                };
                if (element.Type() == ElementType::button) {
                    result.width += 48.0f;
                    result.height += 24.0f;
                }
                element.SetDesiredSize(result);
                return result;
            }

            Size result{};
            for (const auto& child : element.Children()) {
                const Size childSize = measure(*child);
                if (element.OrientationValue() == Orientation::vertical) {
                    result.width = std::max(result.width, childSize.width);
                    result.height += childSize.height;
                } else {
                    result.width += childSize.width;
                    result.height = std::max(result.height, childSize.height);
                }
            }
            if (!element.Children().empty()) {
                // Spacing существует только между соседями, а не после
                // последнего дочернего элемента.
                result.height += element.OrientationValue() == Orientation::vertical
                    ? element.Spacing() * static_cast<float>(element.Children().size() - 1)
                    : 0.0f;
                result.width += element.OrientationValue() == Orientation::horizontal
                    ? element.Spacing() * static_cast<float>(element.Children().size() - 1)
                    : 0.0f;
            }
            element.SetDesiredSize(result);
            return result;
        }

        void arrange(Element& element, Rect bounds) {
            // Второй проход выдаёт каждому элементу конечный прямоугольник
            // сверху вниз. StackPanel центрирует детей по поперечной оси.
            element.SetBounds(bounds);
            float cursor = element.OrientationValue() == Orientation::vertical ? bounds.y : bounds.x;
            for (const auto& child : element.Children()) {
                const Size size = child->DesiredSize();
                Rect childBounds;
                if (element.OrientationValue() == Orientation::vertical) {
                    childBounds = {bounds.x + (bounds.width - size.width) / 2.0f, cursor, size.width, size.height};
                    cursor += size.height + element.Spacing();
                } else {
                    childBounds = {cursor, bounds.y + (bounds.height - size.height) / 2.0f, size.width, size.height};
                    cursor += size.width + element.Spacing();
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