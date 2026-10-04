#pragma once
#include <XamlRuntime/Binding.h>

#include "../../Interface/IPage.h"
#include "../../Core/PageRegistry.h"

#include <map>

namespace {{application}}::application::ui::page {
    class MainPageViewModel final : public interface::IPage {
    public:
        static constexpr std::string_view PageName = "MainPage";
        enum class Property { status, packageVersion };
        using PropertyChangedHandler = std::function<void(Property)>;
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
        xaml::Element::Command RequestApplicationUpdateCommand();
        xaml::Element::Command SendLogsCommand();
        const std::string& Status() const;
        const std::string& PackageVersion() const;
        std::function<void()> Subscribe(PropertyChangedHandler handler);

    private:
        core::PageContext& context;
        std::string status;
        std::string packageVersion;
        std::map<size_t, PropertyChangedHandler> handlers;
        size_t nextSubscription = 0;
        std::unique_ptr<xaml::Element> root;
        std::unique_ptr<xaml::BindingScope> bindings;
    };
}