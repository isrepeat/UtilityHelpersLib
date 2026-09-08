#include "XamlRuntime/UserControl.h"

#include <stdexcept>

namespace xaml {
    UserControl::UserControl()
        : Element(ElementType::grid) {
    }

    UserControl::~UserControl() = default;

    Element* UserControl::Content() const {
        return this->content;
    }

    void UserControl::InitializeComponent(std::unique_ptr<Element> content) {
        if (!content || this->content) {
            throw std::logic_error("UserControl content can only be initialized once");
        }
        this->content = content.get();
        this->AddChild(std::move(content));
        this->OnInitialized();
    }

    void UserControl::OnInitialized() {
    }
}
