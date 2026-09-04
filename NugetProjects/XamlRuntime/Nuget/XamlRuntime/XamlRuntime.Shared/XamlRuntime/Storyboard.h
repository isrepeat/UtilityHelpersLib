#pragma once

#include <chrono>
#include <vector>

namespace xaml {
    enum class AnimatedProperty {
        opacity,
        renderOffsetX,
        toggleProgress,
        pressProgress,
        waveProgress,
        waveOpacity,
    };

    enum class AnimationTrigger {
        pointerDown,
        pointerUp,
        toggled,
    };

    enum class Easing {
        linear,
        cubicOut,
    };

    struct AnimationTrack {
        AnimatedProperty property = AnimatedProperty::opacity;
        float from = 0.0f;
        float to = 0.0f;
        bool fromCurrent = false;
        bool toToggleState = false;
        float intensity = 0.45f;
        float spread = 0.28f;
        float fadeExponent = 2.0f;
        std::chrono::milliseconds duration{};
        Easing easing = Easing::linear;
    };

    struct Storyboard {
        AnimationTrigger trigger = AnimationTrigger::pointerDown;
        std::vector<AnimationTrack> tracks;
    };
}