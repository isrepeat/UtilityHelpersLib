#include "XamlRuntime/XamlLayout.h"
#include <algorithm>

namespace mobileclock::ui::_details {
    float horizontal(const attr::Thickness& thickness) {
        return thickness.left + thickness.right;
    }

    float vertical(const attr::Thickness& thickness) {
        return thickness.top + thickness.bottom;
    }

    Rect inset(Rect bounds, attr::Thickness thickness) {
        return {
            bounds.x + thickness.left,
            bounds.y + thickness.top,
            std::max(0.0f, bounds.width - horizontal(thickness)),
            std::max(0.0f, bounds.height - vertical(thickness)),
        };
    }

    Size withCommonSize(const Element& element, Size contentSize) {
        const attr::Thickness padding = element.Padding();
        const attr::Thickness border = element.BorderThickness();
        const attr::Thickness margin = element.Margin();
        contentSize.width += horizontal(padding) + horizontal(border);
        contentSize.height += vertical(padding) + vertical(border);
        if (element.Width() > 0.0f) {
            contentSize.width = element.Width();
        }
        if (element.Height() > 0.0f) {
            contentSize.height = element.Height();
        }
        contentSize.width += horizontal(margin);
        contentSize.height += vertical(margin);
        return contentSize;
    }

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
            result = withCommonSize(element, result);
            element.SetDesiredSize(result);
            return result;
        }

        if (element.Type() == ElementType::toggleSwitch) {
            Size result{
                element.Width() > 0.0f ? element.Width() : 56.0f,
                element.Height() > 0.0f ? element.Height() : 32.0f,
            };
            result = withCommonSize(element, result);
            element.SetDesiredSize(result);
            return result;
        }

        Size result{};
        for (const auto& child : element.Children()) {
            const Size childSize = measure(*child);
            if (element.OrientationValue() == attr::Orientation::vertical) {
                result.width = std::max(result.width, childSize.width);
                result.height += childSize.height;
            } else {
                result.width += childSize.width;
                result.height = std::max(result.height, childSize.height);
            }
        }
        result = withCommonSize(element, result);
        element.SetDesiredSize(result);
        return result;
    }

    void arrange(Element& element, Rect bounds) {
        // Второй проход выдаёт каждому элементу конечный прямоугольник
        // сверху вниз. StackPanel центрирует детей по поперечной оси.
        const Rect elementBounds = inset(bounds, element.Margin());
        element.SetBounds(elementBounds);
        const attr::Thickness paddingAndBorder{
            element.Padding().left + element.BorderThickness().left,
            element.Padding().right + element.BorderThickness().right,
            element.Padding().top + element.BorderThickness().top,
            element.Padding().bottom + element.BorderThickness().bottom,
        };
        const Rect contentBounds = inset(elementBounds, paddingAndBorder);
        if (element.Type() == ElementType::page) {
            for (const auto& child : element.Children()) {
                const Size childSize = child->DesiredSize();
                arrange(*child, {
                    contentBounds.x + (contentBounds.width - childSize.width) / 2.0f,
                    contentBounds.y + (contentBounds.height - childSize.height) / 2.0f,
                    childSize.width,
                    childSize.height,
                });
            }
            return;
        }
        float cursor = element.OrientationValue() == attr::Orientation::vertical ? contentBounds.y : contentBounds.x;
        for (const auto& child : element.Children()) {
            const Size size = child->DesiredSize();
            Rect childBounds;
            if (element.OrientationValue() == attr::Orientation::vertical) {
                childBounds = {contentBounds.x + (contentBounds.width - size.width) / 2.0f, cursor, size.width, size.height};
                cursor += size.height;
            } else {
                childBounds = {cursor, contentBounds.y + (contentBounds.height - size.height) / 2.0f, size.width, size.height};
                cursor += size.width;
            }
            arrange(*child, childBounds);
        }
    }
}

namespace mobileclock::ui {
    Element::Element(ElementType type)
        : type(type) {
    }

    ElementType Element::Type() const {
        return this->type;
    }

    const std::string& Element::Id() const {
        return this->id;
    }

    void Element::SetId(std::string value) {
        this->id = std::move(value);
    }

    const std::string& Element::Text() const {
        return this->text;
    }

    void Element::SetText(std::string value) {
        this->text = std::move(value);
    }

    float Element::FontSize() const {
        return this->fontSize;
    }

    void Element::SetFontSize(float value) {
        this->fontSize = value;
    }

    const std::string& Element::FontFamily() const {
        return this->fontFamily;
    }

    void Element::SetFontFamily(std::string value) {
        this->fontFamily = std::move(value);
    }

    const std::string& Element::FontWeight() const {
        return this->fontWeight;
    }

    void Element::SetFontWeight(std::string value) {
        this->fontWeight = std::move(value);
    }

    attr::Color Element::Foreground() const {
        return this->foreground;
    }

    void Element::SetForeground(attr::Color value) {
        this->foreground = value;
    }

    attr::Orientation Element::OrientationValue() const {
        return this->orientation;
    }

    void Element::SetOrientation(attr::Orientation value) {
        this->orientation = value;
    }

    attr::Color Element::Background() const {
        return this->background;
    }

    void Element::SetBackground(attr::Color value) {
        this->background = value;
    }

    attr::Color Element::BorderColor() const {
        return this->borderColor;
    }

    void Element::SetBorderColor(attr::Color value) {
        this->borderColor = value;
    }

    attr::Thickness Element::Margin() const {
        return this->margin;
    }

    void Element::SetMargin(attr::Thickness value) {
        this->margin = value;
    }

    attr::Thickness Element::Padding() const {
        return this->padding;
    }

    void Element::SetPadding(attr::Thickness value) {
        this->padding = value;
    }

    attr::Thickness Element::BorderThickness() const {
        return this->borderThickness;
    }

    void Element::SetBorderThickness(attr::Thickness value) {
        this->borderThickness = value;
    }

    float Element::CornerRadius() const {
        return this->cornerRadius;
    }

    void Element::SetCornerRadius(float value) {
        this->cornerRadius = value;
    }

    float Element::Width() const {
        return this->width;
    }

    void Element::SetWidth(float value) {
        this->width = value;
    }

    float Element::Height() const {
        return this->height;
    }

    void Element::SetHeight(float value) {
        this->height = value;
    }

    bool Element::IsOn() const {
        return this->isOn;
    }

    void Element::SetIsOn(bool value) {
        this->isOn = value;
    }

    Size Element::DesiredSize() const {
        return this->desiredSize;
    }

    void Element::SetDesiredSize(Size value) {
        this->desiredSize = value;
    }

    Rect Element::Bounds() const {
        return this->bounds;
    }

    void Element::SetBounds(Rect value) {
        this->bounds = value;
    }

    const std::vector<std::unique_ptr<Element>>& Element::Children() const {
        return this->children;
    }

    void Element::AddChild(std::unique_ptr<Element> child) {
        this->children.push_back(std::move(child));
    }

    void layout(Element& root, Size availableSize) {
        // Разделение measure/arrange позволяет заменить или расширить layout
        // контейнеры, не меняя контракт визуального дерева.
        const Size desired = _details::measure(root);
        if (root.Type() == ElementType::page) {
            _details::arrange(root, {0.0f, 0.0f, availableSize.width, availableSize.height});
            return;
        }
        _details::arrange(root, {
            (availableSize.width - desired.width) / 2.0f,
            (availableSize.height - desired.height) / 2.0f,
            desired.width,
            desired.height,
        });
    }
}