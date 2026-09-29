#pragma once
#include <XamlRuntime/Binding.h>

#include "../../Interface/IPage.h"
#include "../../Core/PageRegistry.h"

namespace {{application}}::application::ui::page {
    class MainPageViewModel final : public interface::IPage {
    public:
        static constexpr std::string_view PageName = "MainPage";
        explicit MainPageViewModel(core::PageContext& context);
        ~MainPageViewModel() = default;

        //
        // INavigationPage
        //
        std::unique_ptr<base::NavigationStateBase> OnNavigatingFrom(const core::NavigationRequest& request) override;
        bool OnNavigatingTo(const core::NavigationRequest& request, std::unique_ptr<base::NavigationStateBase> state) override;

        //
        // IPage
        //
        std::string_view Name() const override;
        std::string_view Title() const override;
        void Initialize(xaml::Size availableSize) override;
        void Update() override;
        xaml::Element& Root() override;
#if defined(ANDROID_APP_PREVIEWER)
        xaml::runtime::RuntimeBindingContext preview_RuntimeContext() override;
        void preview_ReplaceRuntimeTree(xaml::runtime::RuntimeBuildResult result) override;
#endif
        xaml::Element::Command NavigateToSettingsCommand();

    private:
        core::PageContext& context;
        std::unique_ptr<xaml::Element> root;
        std::unique_ptr<xaml::BindingScope> bindings;
    };
}