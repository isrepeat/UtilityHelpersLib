#pragma once

#include <unordered_map>
#include <string>
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
        show,
        hide,
    };

    enum class Easing {
        linear,
        cubicOut,
    };

    class AnimationSettings final {
    public:
        void Set(std::string name, std::string value);
        const std::string& Get(const std::string& name) const;
        float Number(const std::string& name, float fallback) const;
        const std::unordered_map<std::string, std::string>& Values() const;

    private:
        std::unordered_map<std::string, std::string> values;
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
        std::string name;
        AnimationSettings settings;
    };

    struct Storyboard {
        AnimationTrigger trigger = AnimationTrigger::pointerDown;
        std::vector<AnimationTrack> tracks;
    };
}