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

        ElementType type() const { return type_; }
        const std::string& id() const { return id_; }
        const std::string& text() const { return text_; }
        float fontSize() const { return fontSize_; }
        Color foreground() const { return foreground_; }
        Orientation orientation() const { return orientation_; }
        float spacing() const { return spacing_; }
        Size desiredSize() const { return desiredSize_; }
        Rect bounds() const { return bounds_; }
        const std::vector<std::unique_ptr<Element>>& children() const { return children_; }

        void setId(std::string value) { id_ = std::move(value); }
        void setText(std::string value) { text_ = std::move(value); }
        void setFontSize(float value) { fontSize_ = value; }
        void setForeground(Color value) { foreground_ = value; }
        void setOrientation(Orientation value) { orientation_ = value; }
        void setSpacing(float value) { spacing_ = value; }
        void addChild(std::unique_ptr<Element> child) { children_.push_back(std::move(child)); }
        void setDesiredSize(Size value) { desiredSize_ = value; }
        void setBounds(Rect value) { bounds_ = value; }

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

    std::unique_ptr<Element> createMainPage();
    void layout(Element& root, Size availableSize);
}