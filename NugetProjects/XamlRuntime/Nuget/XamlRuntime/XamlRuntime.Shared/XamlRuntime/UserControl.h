#pragma once
#include "XamlLayout.h"

#include <memory>

namespace xaml {
    class UserControl : public Element {
    public:
        UserControl();
        ~UserControl() override;

        Element* Content() const;

    protected:
        void InitializeComponent(std::unique_ptr<Element> content);
        void ReplaceContent(std::unique_ptr<Element> content);
        virtual void OnInitialized();

    private:
        Element* content = nullptr;
    };
}