#include "ApplicationSession.h"

#include <utility>

namespace {{application}}::application::core {
    ApplicationSession::ApplicationSession(model::ApplicationStateDocument applicationStateDocument, ApplicationStateStore::DocumentSaveHandler save)
        : stateStore(std::move(applicationStateDocument), std::move(save))
        , applicationRepository(this->stateStore)
        , pageManager(this->applicationRepository, this->appSessionController) {
    }

    //
    // API
    //
    AppSessionController& ApplicationSession::Controller() {
        return this->appSessionController;
    }

    void ApplicationSession::Initialize(xaml::Size size) {
        this->pageManager.Initialize(size);
    }

    void ApplicationSession::Resize(xaml::Size size) {
        this->pageManager.Resize(size);
    }

    PageManager& ApplicationSession::Pages() {
        return this->pageManager;
    }

    const PageManager& ApplicationSession::Pages() const {
        return this->pageManager;
    }

    model::ApplicationRepository& ApplicationSession::Repository() {
        return this->applicationRepository;
    }

    xaml::Element& ApplicationSession::Root() {
        return this->pageManager.Root();
    }

    void ApplicationSession::PointerDown(float x, float y) {
        this->pageManager.PointerDown(x, y);
    }

    void ApplicationSession::PointerMove(float x, float y) {
        this->pageManager.PointerMove(x, y);
    }

    void ApplicationSession::PointerUp(float x, float y) {
        this->pageManager.PointerUp(x, y);
    }

    void ApplicationSession::CancelPointer() {
        this->pageManager.CancelPointer();
    }

    bool ApplicationSession::Update() {
        return this->pageManager.Update();
    }

    void ApplicationSession::Render(xaml::IRenderBackend& renderer) {
        this->pageManager.Render(renderer, this->renderers);
    }
}