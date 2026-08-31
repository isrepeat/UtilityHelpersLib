#pragma once

#include "XamlRuntime/XamlLayout.h"

#include <functional>
#include <utility>
#include <vector>

namespace xaml {
    class BindingScope final {
    public:
        BindingScope() = default;
        ~BindingScope();

        BindingScope(const BindingScope&) = delete;
        BindingScope& operator=(const BindingScope&) = delete;

        void Clear();
        void UpdateSource(Element& element) const;

        template <typename TViewModel, typename TGetter, typename TSetter, typename TProperty>
        void AddOneWay(
            Element& element,
            TViewModel& viewModel,
            TGetter getter,
            TSetter setter,
            TProperty property) {
            const auto apply = [&element, &viewModel, getter, setter]() {
                std::invoke(setter, element, std::invoke(getter, viewModel));
            };
            apply();
            this->unsubscriptions.push_back(viewModel.Subscribe([apply, property](TProperty changedProperty) {
                if (changedProperty == property) {
                    apply();
                }
            }));
        }

        template <typename TViewModel, typename TSourceGetter, typename TSourceSetter,
            typename TTargetGetter, typename TTargetSetter, typename TProperty>
        void AddTwoWay(
            Element& element,
            TViewModel& viewModel,
            TSourceGetter sourceGetter,
            TSourceSetter sourceSetter,
            TTargetGetter targetGetter,
            TTargetSetter targetSetter,
            TProperty property) {
            this->AddOneWay(element, viewModel, sourceGetter, targetSetter, property);
            this->sourceUpdates.push_back({&element, [&element, &viewModel, sourceSetter, targetGetter]() {
                std::invoke(sourceSetter, viewModel, std::invoke(targetGetter, element));
            }});
        }

    private:
        struct SourceUpdate {
            Element* element = nullptr;
            std::function<void()> update;
        };

    private:
        std::vector<std::function<void()>> unsubscriptions;
        std::vector<SourceUpdate> sourceUpdates;
    };
}
