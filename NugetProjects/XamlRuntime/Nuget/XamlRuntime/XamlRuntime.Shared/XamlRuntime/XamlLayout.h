#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mobileclock::ui {
    enum class ElementType {
        stackPanel,
        textBlock,
        button,
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
        explicit Element(ElementType type) : type_(type) {}

        ElementType Type() const { return type_; }
        const std::string& Id() const { return id_; }
        const std::string& Text() const { return text_; }
        float FontSize() const { return fontSize_; }
        Color Foreground() const { return foreground_; }
        Orientation OrientationValue() const { return orientation_; }
        float Spacing() const { return spacing_; }
        Size DesiredSize() const { return desiredSize_; }
        Rect Bounds() const { return bounds_; }
        const std::vector<std::unique_ptr<Element>>& Children() const { return children_; }

        void SetId(std::string value) { id_ = std::move(value); }
        void SetText(std::string value) { text_ = std::move(value); }
        void SetFontSize(float value) { fontSize_ = value; }
        void SetForeground(Color value) { foreground_ = value; }
        void SetOrientation(Orientation value) { orientation_ = value; }
        void SetSpacing(float value) { spacing_ = value; }
        void AddChild(std::unique_ptr<Element> child) { children_.push_back(std::move(child)); }
        void SetDesiredSize(Size value) { desiredSize_ = value; }
        void SetBounds(Rect value) { bounds_ = value; }

    private:
        ElementType type_;
        std::string id_;
        std::string text_;
        float fontSize_ = 16.0f;
        Color foreground_{};
        Orientation orientation_ = Orientation::vertical;
        float spacing_ = 0.0f;
        Size desiredSize_{};
        Rect bounds_{};
        std::vector<std::unique_ptr<Element>> children_;
    };

    void layout(Element& root, Size availableSize);
}