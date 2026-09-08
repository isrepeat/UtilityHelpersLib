#pragma once

#include "XamlRuntime/XamlLayout.h"

#include <memory>

namespace xaml {
    class UserControl : public Element {
    public:
        UserControl();
        ~UserControl() override;

        Element* Content() const;

    protected:
        void InitializeComponent(std::unique_ptr<Element> content);
        virtual void OnInitialized();

    private:
        Element* content = nullptr;
    };
}
