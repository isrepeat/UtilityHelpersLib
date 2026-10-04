#pragma once
#include <XamlRuntime/RenderEngine.h>

#include "./ApplicationStateStore.h"
#include "./AppSessionController.h"
#include "./PageManager.h"

namespace {{application}}::application::core {
    class ApplicationSession final {
    public:
        explicit ApplicationSession(model::ApplicationStateDocument document = {}, ApplicationStateStore::DocumentSaveHandler save = {});
        ~ApplicationSession() = default;
        ApplicationSession(const ApplicationSession&) = delete;
        ApplicationSession& operator=(const ApplicationSession&) = delete;
        AppSessionController& Controller();
        void Initialize(xaml::Size size);
        void Resize(xaml::Size size);
        PageManager& Pages();
        const PageManager& Pages() const;
        model::ApplicationRepository& Repository();
        xaml::Element& Root();
        void PointerDown(float x, float y);
        void PointerMove(float x, float y);
        void PointerUp(float x, float y);
        void CancelPointer();
        bool Update();
        void Render(xaml::IRenderBackend& renderer);

    private:
        ApplicationStateStore stateStore;
        model::ApplicationRepository repository;
        AppSessionController controller;
        PageManager pageManager;
        xaml::RendererRegistry renderers;
    };
}