#include <Helpers.Logging/Logging.h>

#include "XamlLayout.h"

#include <algorithm>
#include <sstream>

namespace xaml::_details {
    size_t utf8Length(const std::string& text) {
        size_t result = 0;
        for (const unsigned char character : text) {
            if ((character & 0xC0) != 0x80) {
                ++result;
            }
        }
        return result;
    }

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

    Rect intersect(Rect first, Rect second) {
        const float left = std::max(first.x, second.x);
        const float top = std::max(first.y, second.y);
        const float right = std::min(first.x + first.width, second.x + second.width);
        const float bottom = std::min(first.y + first.height, second.y + second.height);
        return {
            left,
            top,
            std::max(0.0f, right - left),
            std::max(0.0f, bottom - top),
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
        if (element.VisibilityValue() == attr::Visibility::collapsed) {
            element.SetDesiredSize({});
            return {};
        }
        // Первый проход вычисляет требуемый размер снизу вверх. Точная
        // метрика шрифта появится позже; пока ширина текста оценивается.
        if (element.Type() == ElementType::textBlock || element.Type() == ElementType::button) {
            Size result{
                std::max(1.0f, static_cast<float>(utf8Length(element.Text())) * element.FontSize() * 0.55f),
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
            if (element.Type() == ElementType::grid) {
                result.width = std::max(result.width, childSize.width);
                result.height = std::max(result.height, childSize.height);
            } else if (element.OrientationValue() == attr::Orientation::vertical) {
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

    float alignedOffset(attr::Alignment alignment, float available, float desired) {
        if (alignment == attr::Alignment::right || alignment == attr::Alignment::bottom) {
            return available - desired;
        }
        if (alignment == attr::Alignment::center) {
            return (available - desired) / 2.0f;
        }
        return 0.0f;
    }

    float alignedSize(attr::Alignment alignment, float available, float desired) {
        return alignment == attr::Alignment::stretch ? available : desired;
    }

    std::vector<std::string> tracks(const std::string& definitions) {
        std::vector<std::string> result;
        std::istringstream input(definitions);
        std::string track;
        while (std::getline(input, track, ',')) {
            result.push_back(track);
        }
        if (result.empty()) {
            result.push_back("*");
        }
        return result;
    }

    std::vector<float> trackSizes(
        const std::vector<std::string>& definitions,
        float available,
        const Element& grid,
        bool columns) {
        std::vector<float> sizes(definitions.size());
        float used = 0.0f;
        size_t starCount = 0;
        for (size_t index = 0; index < definitions.size(); ++index) {
            const std::string& definition = definitions[index];
            if (definition == "*") {
                ++starCount;
            } else if (!definition.empty() && definition.back() == '%') {
                sizes[index] = available * std::stof(definition.substr(0, definition.size() - 1)) / 100.0f;
                used += sizes[index];
            } else if (definition == "Auto") {
                for (const auto& child : grid.Children()) {
                    const int track = columns ? child->GridColumn() : child->GridRow();
                    if (track == static_cast<int>(index)) {
                        sizes[index] = std::max(sizes[index], columns
                            ? child->DesiredSize().width : child->DesiredSize().height);
                    }
                }
                used += sizes[index];
            }
        }
        const float starSize = starCount == 0 ? 0.0f : std::max(0.0f, available - used) / starCount;
        for (size_t index = 0; index < definitions.size(); ++index) {
            if (definitions[index] == "*") {
                sizes[index] = starSize;
            }
        }
        return sizes;
    }

    void arrange(Element& element, Rect bounds, Rect parentClipBounds) {
        // Второй проход выдаёт каждому элементу конечный прямоугольник
        // сверху вниз. StackPanel центрирует детей по поперечной оси.
        const Rect elementBounds = inset(bounds, element.Margin());
        element.SetBounds(elementBounds);
        element.SetClipBounds(intersect(elementBounds, parentClipBounds));
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
                    contentBounds.x + alignedOffset(
                        child->HorizontalAlignmentValue(), contentBounds.width, childSize.width),
                    contentBounds.y + alignedOffset(
                        child->VerticalAlignmentValue(), contentBounds.height, childSize.height),
                    alignedSize(child->HorizontalAlignmentValue(), contentBounds.width, childSize.width),
                    alignedSize(child->VerticalAlignmentValue(), contentBounds.height, childSize.height),
                }, element.ClipBounds());
            }
            return;
        }
        if (element.Type() == ElementType::grid) {
            const std::vector<std::string> columns = tracks(element.Columns());
            const std::vector<std::string> rows = tracks(element.Rows());
            const std::vector<float> columnSizes = trackSizes(columns, contentBounds.width, element, true);
            const std::vector<float> rowSizes = trackSizes(rows, contentBounds.height, element, false);
            for (const auto& child : element.Children()) {
                const size_t column = std::min(static_cast<size_t>(std::max(0, child->GridColumn())), columns.size() - 1);
                const size_t row = std::min(static_cast<size_t>(std::max(0, child->GridRow())), rows.size() - 1);
                float x = contentBounds.x;
                for (size_t index = 0; index < column; ++index) {
                    x += columnSizes[index];
                }
                float y = contentBounds.y;
                for (size_t index = 0; index < row; ++index) {
                    y += rowSizes[index];
                }
                const Size childSize = child->DesiredSize();
                arrange(*child, {
                    x + alignedOffset(child->HorizontalAlignmentValue(), columnSizes[column], childSize.width),
                    y + alignedOffset(child->VerticalAlignmentValue(), rowSizes[row], childSize.height),
                    alignedSize(child->HorizontalAlignmentValue(), columnSizes[column], childSize.width),
                    alignedSize(child->VerticalAlignmentValue(), rowSizes[row], childSize.height),
                }, element.ClipBounds());
            }
            return;
        }
        float cursor = element.OrientationValue() == attr::Orientation::vertical ? contentBounds.y : contentBounds.x;
        for (const auto& child : element.Children()) {
            const Size size = child->DesiredSize();
            Rect childBounds;
            if (element.OrientationValue() == attr::Orientation::vertical) {
                childBounds = {contentBounds.x + alignedOffset(child->HorizontalAlignmentValue(), contentBounds.width, size.width), cursor, alignedSize(child->HorizontalAlignmentValue(), contentBounds.width, size.width), size.height};
                cursor += size.height;
            } else {
                childBounds = {cursor, contentBounds.y + alignedOffset(child->VerticalAlignmentValue(), contentBounds.height, size.height), size.width, alignedSize(child->VerticalAlignmentValue(), contentBounds.height, size.height)};
                cursor += size.width;
            }
            arrange(*child, childBounds, element.ClipBounds());
        }
    }
}

namespace xaml {
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
        if (this->text == value) {
            return;
        }
        this->text = std::move(value);
        this->InvalidateLayout();
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

    const std::string& Element::Source() const {
        return this->source;
    }

    void Element::SetSource(std::string value) {
        this->source = std::move(value);
    }

    attr::Color Element::Tint() const {
        return this->tint;
    }

    void Element::SetTint(attr::Color value) {
        this->tint = value;
    }

    const std::string& Element::Command() const {
        return this->command;
    }

    void Element::SetCommand(std::string value) {
        this->command = std::move(value);
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

    attr::Alignment Element::VerticalAlignmentValue() const {
        return this->verticalAlignment;
    }

    void Element::SetVerticalAlignment(attr::Alignment value) {
        this->verticalAlignment = value;
    }

    attr::Alignment Element::HorizontalAlignmentValue() const {
        return this->horizontalAlignment;
    }

    void Element::SetHorizontalAlignment(attr::Alignment value) {
        this->horizontalAlignment = value;
    }

    attr::Alignment Element::ContentAlignmentValue() const {
        return this->contentAlignment;
    }

    void Element::SetContentAlignment(attr::Alignment value) {
        this->contentAlignment = value;
    }

    int Element::GridRow() const {
        return this->gridRow;
    }

    void Element::SetGridRow(int value) {
        this->gridRow = value;
    }

    int Element::GridColumn() const {
        return this->gridColumn;
    }

    void Element::SetGridColumn(int value) {
        this->gridColumn = value;
    }

    const std::string& Element::Rows() const {
        return this->rows;
    }

    void Element::SetRows(std::string value) {
        this->rows = std::move(value);
    }

    const std::string& Element::Columns() const {
        return this->columns;
    }

    void Element::SetColumns(std::string value) {
        this->columns = std::move(value);
    }

    attr::Color Element::Background() const {
        return this->background;
    }

    void Element::SetBackground(attr::Color value) {
        this->background = value;
    }

    attr::Color Element::ActiveBackground() const {
        return this->activeBackground;
    }

    void Element::SetActiveBackground(attr::Color value) {
        this->activeBackground = value;
    }

    attr::Color Element::BorderColor() const {
        return this->borderColor;
    }

    void Element::SetBorderColor(attr::Color value) {
        this->borderColor = value;
    }

    attr::Color Element::ActiveBorderColor() const {
        return this->activeBorderColor;
    }

    void Element::SetActiveBorderColor(attr::Color value) {
        this->activeBorderColor = value;
    }

    attr::Color Element::ActiveForeground() const {
        return this->activeForeground;
    }

    void Element::SetActiveForeground(attr::Color value) {
        this->activeForeground = value;
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

    attr::Visibility Element::VisibilityValue() const {
        return this->visibility;
    }

    void Element::SetVisibility(attr::Visibility value) {
        this->visibility = value;
    }

    bool Element::IsEnabled() const {
        return this->isEnabled;
    }

    void Element::SetIsEnabled(bool value) {
        this->isEnabled = value;
    }

    float Element::Opacity() const {
        return this->opacity;
    }

    void Element::SetOpacity(float value) {
        this->opacity = value;
    }

    float Element::RenderOffsetX() const {
        return this->renderOffsetX;
    }

    void Element::SetRenderOffsetX(float value) {
        this->renderOffsetX = value;
    }

    float Element::ToggleProgress() const {
        return this->toggleProgress;
    }

    void Element::SetToggleProgress(float value) {
        this->toggleProgress = value;
    }

    float Element::PressProgress() const {
        return this->pressProgress;
    }

    void Element::SetPressProgress(float value) {
        this->pressProgress = value;
    }

    float Element::WaveProgress() const {
        return this->waveProgress;
    }

    void Element::SetWaveProgress(float value) {
        this->waveProgress = value;
    }

    float Element::WaveOpacity() const {
        return this->waveOpacity;
    }

    void Element::SetWaveOpacity(float value) {
        this->waveOpacity = value;
    }

    float Element::WaveIntensity() const {
        return this->waveIntensity;
    }

    void Element::SetWaveIntensity(float value) {
        this->waveIntensity = value;
    }

    float Element::WaveSpread() const {
        return this->waveSpread;
    }

    void Element::SetWaveSpread(float value) {
        this->waveSpread = value;
    }

    float Element::WaveFadeExponent() const {
        return this->waveFadeExponent;
    }

    void Element::SetWaveFadeExponent(float value) {
        this->waveFadeExponent = value;
    }

    const std::vector<Storyboard>& Element::Storyboards() const {
        return this->storyboards;
    }

    void Element::AddStoryboard(Storyboard value) {
        this->storyboards.push_back(std::move(value));
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

    Rect Element::ClipBounds() const {
        return this->clipBounds;
    }

    void Element::SetClipBounds(Rect value) {
        this->clipBounds = value;
    }

    const std::vector<std::unique_ptr<Element>>& Element::Children() const {
        return this->children;
    }

    std::vector<std::unique_ptr<Element>>& Element::Children() {
        return this->children;
    }

    void Element::AddChild(std::unique_ptr<Element> child) {
        child->parent = this;
        this->children.push_back(std::move(child));
        this->InvalidateLayout();
    }

    void Element::InvalidateLayout() {
        this->layoutInvalid = true;
        if (this->parent != nullptr) {
            this->parent->InvalidateLayout();
        }
    }

    Element& ElementAt(Element& root, std::initializer_list<size_t> path) {
        Element* element = &root;
        for (const size_t childIndex : path) {
            element = element->Children().at(childIndex).get();
        }
        return *element;
    }

    void layout(Element& root, Size availableSize) {
        // Разделение measure/arrange позволяет заменить или расширить layout
        // контейнеры, не меняя контракт визуального дерева.
        const Size desired = _details::measure(root);
        // Layout вызывается только при invalidation, а не в каждом render frame.
        // Поэтому запись полезна для диагностики перестроений и не создаёт
        // постоянной нагрузки в render loop.
        LOG_DEBUG(
            "XamlRuntime.Layout",
            "Layout: root='{}', available={}x{}, desired={}x{}",
            root.Id(),
            availableSize.width,
            availableSize.height,
            desired.width,
            desired.height);
        if (root.Type() == ElementType::page) {
            _details::arrange(
                root,
                {0.0f, 0.0f, availableSize.width, availableSize.height},
                {0.0f, 0.0f, availableSize.width, availableSize.height});
            root.availableSize = availableSize;
            root.layoutInvalid = false;
            return;
        }
        const Rect rootBounds{
            (availableSize.width - desired.width) / 2.0f,
            (availableSize.height - desired.height) / 2.0f,
            desired.width,
            desired.height,
        };
        _details::arrange(root, rootBounds, rootBounds);
        root.availableSize = availableSize;
        root.layoutInvalid = false;
    }
}