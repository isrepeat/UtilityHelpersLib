#pragma once

#include <chrono>
#include <vector>

namespace xaml {
    class Element;

    enum class AnimatedProperty {
        opacity,
        renderOffsetX,
        toggleProgress,
        pressProgress,
    };

    enum class Easing {
        linear,
        cubicOut,
    };

    class AnimationController final {
    public:
        void Animate(
            Element& target,
            AnimatedProperty property,
            float from,
            float to,
            std::chrono::milliseconds duration,
            Easing easing = Easing::cubicOut);
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
            std::chrono::steady_clock::time_point startedAt{};
        };

    private:
        std::vector<Animation> animations;
    };
}