#pragma once
#include <XamlRuntime/InteractionController.h>
#include <XamlRuntime/Animation.h>

namespace {{application}}::application::core {
    class InputDispatcher final {
    public:
        void PointerDown(xaml::Element& root, xaml::AnimationController& animations, float x, float y);
        void PointerMove(float x, float y);
        void PointerUp(xaml::Element& root, xaml::AnimationController& animations, float x, float y);
        void Cancel();
        bool Update();

    private:
        xaml::InteractionController interaction;
    };
}