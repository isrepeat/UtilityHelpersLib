#include <Helpers.Logging/Logging.h>

#include "Animation.h"
#include "XamlLayout.h"

#include <algorithm>

namespace xaml::_details {
    float Ease(float progress, Easing easing) {
        if (easing == Easing::cubicOut) {
            const float inverse = 1.0f - progress;
            return 1.0f - inverse * inverse * inverse;
        }
        return progress;
    }

    void SetAnimatedValue(Element& target, AnimatedProperty property, float value) {
        if (property == AnimatedProperty::opacity) {
            target.SetOpacity(value);
        } else if (property == AnimatedProperty::renderOffsetX) {
            target.SetRenderOffsetX(value);
        } else if (property == AnimatedProperty::toggleProgress) {
            target.SetToggleProgress(value);
        } else {
            target.SetPressProgress(value);
        }
    }
}

namespace xaml {
    void AnimationController::Animate(
        Element& target,
        AnimatedProperty property,
        float from,
        float to,
        std::chrono::milliseconds duration,
        Easing easing) {
        this->animations.erase(
            std::remove_if(this->animations.begin(), this->animations.end(),
                [&target, property](const Animation& animation) {
                    return animation.target == &target && animation.property == property;
                }),
            this->animations.end());
        _details::SetAnimatedValue(target, property, from);
        this->animations.push_back({
            &target,
            property,
            from,
            to,
            duration,
            easing,
            std::chrono::steady_clock::now(),
        });
        LOG_DEBUG("XamlRuntime.Animation", "Started animation: {} -> {}, {} ms", from, to, duration.count());
    }

    void AnimationController::Update() {
        const auto now = std::chrono::steady_clock::now();
        auto animation = this->animations.begin();
        while (animation != this->animations.end()) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - animation->startedAt);
            const float progress = animation->duration.count() == 0 ? 1.0f : std::min(
                1.0f,
                static_cast<float>(elapsed.count()) / animation->duration.count());
            const float eased = _details::Ease(progress, animation->easing);
            _details::SetAnimatedValue(
                *animation->target,
                animation->property,
                animation->from + (animation->to - animation->from) * eased);
            if (progress < 1.0f) {
                ++animation;
            } else {
                LOG_DEBUG("XamlRuntime.Animation", "Completed animation");
                animation = this->animations.erase(animation);
            }
        }
    }

    bool AnimationController::IsAnimating() const {
        return !this->animations.empty();
    }
}