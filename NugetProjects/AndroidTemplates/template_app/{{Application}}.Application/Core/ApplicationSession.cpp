#include "ApplicationSession.h"

#include <utility>

namespace {{application}}::application::core {
    ApplicationSession::ApplicationSession(model::ApplicationStateDocument document, ApplicationStateStore::DocumentSaveHandler save)
        : stateStore(std::move(document), std::move(save))
        , repository(this->stateStore)
        , pageManager(this->repository, this->controller) {
    }

    //
    // API
    //
    AppSessionController& ApplicationSession::Controller() {
        return this->controller;
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
        return this->repository;
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
        xaml::Render(this->Root(), renderer, this->renderers);
    }
}