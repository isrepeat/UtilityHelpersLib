#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mobileclock::ui {
    enum class ElementType {
        page,
        stackPanel,
        textBlock,
        button,
        border,
        toggleSwitch,
    };

    namespace attr {
        enum class Alignment {
            center,
            top,
        };

        enum class BindingMode {
            oneWay,
            twoWay,
        };

        struct Binding {
            std::string property;
            std::string path;
            BindingMode mode = BindingMode::oneWay;
        };

        enum class Orientation {
            horizontal,
            vertical,
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

        attr::Color Foreground() const;
        void SetForeground(attr::Color value);

        attr::Orientation OrientationValue() const;
        void SetOrientation(attr::Orientation value);

        attr::Alignment VerticalAlignmentValue() const;
        void SetVerticalAlignment(attr::Alignment value);

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

        const std::vector<attr::Binding>& Bindings() const;
        void AddBinding(attr::Binding value);

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
        attr::Color foreground{};
        attr::Orientation orientation = attr::Orientation::vertical;
        attr::Alignment verticalAlignment = attr::Alignment::center;
        attr::Color background{0.0f, 0.0f, 0.0f, 0.0f};
        attr::Color borderColor{0.0f, 0.0f, 0.0f, 0.0f};
        attr::Thickness margin{};
        attr::Thickness padding{};
        attr::Thickness borderThickness{};
        float cornerRadius = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        bool isOn = false;
        std::vector<attr::Binding> bindings;
        Size desiredSize{};
        Rect bounds{};
        std::vector<std::unique_ptr<Element>> children;
    };

    void layout(Element& root, Size availableSize);
}
