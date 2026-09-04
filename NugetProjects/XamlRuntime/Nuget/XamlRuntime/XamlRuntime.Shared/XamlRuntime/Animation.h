#pragma once

#include "XamlRuntime/Storyboard.h"

namespace xaml {
    class Element;

    class AnimationController final {
    public:
        void Animate(
            Element& target,
            AnimatedProperty property,
            float from,
            float to,
            std::chrono::milliseconds duration,
            Easing easing = Easing::cubicOut);
        void Start(Element& target, AnimationTrigger trigger);
        void SetPlaybackRate(float value);
        void Update();
        bool IsAnimating() const;

    private:
        struct Animation {
            Element* target = nullptr;
            AnimatedProperty property = AnimatedProperty::opacity;
            float from = 0.0f;
            float to = 0.0f;
            std::chrono::milliseconds duration{};
            Easing easing = Easing::linear;
            float elapsedMilliseconds = 0.0f;
        };

    private:
        std::vector<Animation> animations;
        float playbackRate = 1.0f;
        std::chrono::steady_clock::time_point lastUpdatedAt{};
    };
}