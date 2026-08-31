#pragma once

#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace xaml {
    enum class ElementType {
        page,
        stackPanel,
        textBlock,
        button,
        border,
        toggleSwitch,
        grid,
        scrollViewer,
        image,
        svgImage,
        iconButton,
        listView,
    };

    namespace attr {
        enum class Alignment {
            stretch,
            left,
            right,
            center,
            top,
            bottom,
        };

        enum class Orientation {
            horizontal,
            vertical,
        };

        enum class Visibility {
            collapsed,
            hidden,
            visible,
        };

        struct Color {
            float red = 1.0f;
            float green = 1.0f;
            float blue = 1.0f;
            float alpha = 1.0f;
        };

        struct Thickness {
            float left = 0.0f;
            float right = 0.0f;
            float top = 0.0f;
            float bottom = 0.0f;
        };
    }

    struct Size {
        float width = 0.0f;
        float height = 0.0f;
    };

    struct Rect {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    class Element {
    public:
        explicit Element(ElementType type);

        ElementType Type() const;

        const std::string& Id() const;
        void SetId(std::string value);

        const std::string& Text() const;
        void SetText(std::string value);

        float FontSize() const;
        void SetFontSize(float value);

        const std::string& FontFamily() const;
        void SetFontFamily(std::string value);

        const std::string& FontWeight() const;
        void SetFontWeight(std::string value);

        const std::string& Source() const;
        void SetSource(std::string value);

        attr::Color Tint() const;
        void SetTint(attr::Color value);

        const std::string& Command() const;
        void SetCommand(std::string value);

        attr::Color Foreground() const;
        void SetForeground(attr::Color value);

        attr::Orientation OrientationValue() const;
        void SetOrientation(attr::Orientation value);

        attr::Alignment VerticalAlignmentValue() const;
        void SetVerticalAlignment(attr::Alignment value);

        attr::Alignment HorizontalAlignmentValue() const;
        void SetHorizontalAlignment(attr::Alignment value);

        int GridRow() const;
        void SetGridRow(int value);

        int GridColumn() const;
        void SetGridColumn(int value);

        const std::string& Rows() const;
        void SetRows(std::string value);

        const std::string& Columns() const;
        void SetColumns(std::string value);

        attr::Color Background() const;
        void SetBackground(attr::Color value);

        attr::Color BorderColor() const;
        void SetBorderColor(attr::Color value);

        attr::Thickness Margin() const;
        void SetMargin(attr::Thickness value);

        attr::Thickness Padding() const;
        void SetPadding(attr::Thickness value);

        attr::Thickness BorderThickness() const;
        void SetBorderThickness(attr::Thickness value);

        float CornerRadius() const;
        void SetCornerRadius(float value);

        float Width() const;
        void SetWidth(float value);

        float Height() const;
        void SetHeight(float value);

        bool IsOn() const;
        void SetIsOn(bool value);

        attr::Visibility VisibilityValue() const;
        void SetVisibility(attr::Visibility value);

        bool IsEnabled() const;
        void SetIsEnabled(bool value);

        float Opacity() const;
        void SetOpacity(float value);

        Size DesiredSize() const;
        void SetDesiredSize(Size value);

        Rect Bounds() const;
        void SetBounds(Rect value);

        const std::vector<std::unique_ptr<Element>>& Children() const;
        std::vector<std::unique_ptr<Element>>& Children();
        void AddChild(std::unique_ptr<Element> child);

    private:
        ElementType type;
        std::string id;
        std::string text;
        float fontSize = 16.0f;
        std::string fontFamily;
        std::string fontWeight;
        std::string source;
        attr::Color tint{1.0f, 1.0f, 1.0f, 1.0f};
        std::string command;
        attr::Color foreground{};
        attr::Orientation orientation = attr::Orientation::vertical;
        attr::Alignment verticalAlignment = attr::Alignment::center;
        attr::Alignment horizontalAlignment = attr::Alignment::center;
        int gridRow = 0;
        int gridColumn = 0;
        std::string rows;
        std::string columns;
        attr::Color background{0.0f, 0.0f, 0.0f, 0.0f};
        attr::Color borderColor{0.0f, 0.0f, 0.0f, 0.0f};
        attr::Thickness margin{};
        attr::Thickness padding{};
        attr::Thickness borderThickness{};
        float cornerRadius = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        bool isOn = false;
        attr::Visibility visibility = attr::Visibility::visible;
        bool isEnabled = true;
        float opacity = 1.0f;
        Size desiredSize{};
        Rect bounds{};
        std::vector<std::unique_ptr<Element>> children;
    };

    // Проходит от root по индексам дочерних элементов из path: {1, 1} означает
    // root.Children()[1]->Children()[1]. Пустой путь возвращает сам root.
    Element& ElementAt(Element& root, std::initializer_list<size_t> path);

    void layout(Element& root, Size availableSize);
}