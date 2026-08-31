#include "XamlRuntime/Binding.h"

namespace xaml {
    BindingScope::~BindingScope() {
        this->Clear();
    }

    //
    // API
    //
    void BindingScope::Clear() {
        for (const std::function<void()>& unsubscribe : this->unsubscriptions) {
            unsubscribe();
        }
        this->unsubscriptions.clear();
        this->sourceUpdates.clear();
    }

    void BindingScope::UpdateSource(Element& element) const {
        for (const SourceUpdate& sourceUpdate : this->sourceUpdates) {
            if (sourceUpdate.element == &element) {
                sourceUpdate.update();
            }
        }
    }
}
