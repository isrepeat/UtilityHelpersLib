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
        } else if (property == AnimatedProperty::waveProgress) {
            target.SetWaveProgress(value);
        } else if (property == AnimatedProperty::waveOpacity) {
            target.SetWaveOpacity(value);
        } else {
            target.SetPressProgress(value);
        }
    }

    float AnimatedValue(const Element& target, AnimatedProperty property) {
        if (property == AnimatedProperty::opacity) {
            return target.Opacity();
        }
        if (property == AnimatedProperty::renderOffsetX) {
            return target.RenderOffsetX();
        }
        if (property == AnimatedProperty::toggleProgress) {
            return target.ToggleProgress() < 0.0f
                ? (target.IsOn() ? 1.0f : 0.0f) : target.ToggleProgress();
        }
        if (property == AnimatedProperty::waveProgress) {
            return target.WaveProgress();
        }
        if (property == AnimatedProperty::waveOpacity) {
            return target.WaveOpacity();
        }
        return target.PressProgress();
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
        if (this->animations.empty()) {
            this->lastUpdatedAt = std::chrono::steady_clock::now();
        }
        _details::SetAnimatedValue(target, property, from);
        this->animations.push_back({
            &target,
            property,
            from,
            to,
            duration,
            easing,
            0.0f,
        });
        LOG_DEBUG("XamlRuntime.Animation", "Started animation: {} -> {}, {} ms", from, to, duration.count());
    }

    void AnimationController::Start(Element& target, AnimationTrigger trigger) {
        for (const Storyboard& storyboard : target.Storyboards()) {
            if (storyboard.trigger != trigger) {
                continue;
            }

            for (const AnimationTrack& track : storyboard.tracks) {
                if (track.property == AnimatedProperty::waveProgress) {
                    target.SetWaveIntensity(track.intensity);
                    target.SetWaveSpread(track.spread);
                    target.SetWaveFadeExponent(track.fadeExponent);
                }
                this->Animate(
                    target,
                    track.property,
                    track.fromCurrent ? _details::AnimatedValue(target, track.property) : track.from,
                    track.toToggleState ? (target.IsOn() ? 1.0f : 0.0f) : track.to,
                    track.duration,
                    track.easing);
            }
        }
    }

    void AnimationController::SetPlaybackRate(float value) {
        this->playbackRate = std::clamp(value, 0.1f, 4.0f);
        this->lastUpdatedAt = std::chrono::steady_clock::now();
    }

    void AnimationController::Update() {
        const auto now = std::chrono::steady_clock::now();
        const float elapsedMilliseconds = this->lastUpdatedAt.time_since_epoch().count() == 0
            ? 0.0f
            : std::chrono::duration<float, std::milli>(now - this->lastUpdatedAt).count()
                * this->playbackRate;
        this->lastUpdatedAt = now;
        auto animation = this->animations.begin();
        while (animation != this->animations.end()) {
            animation->elapsedMilliseconds += elapsedMilliseconds;
            const float progress = animation->duration.count() == 0 ? 1.0f : std::min(
                1.0f,
                animation->elapsedMilliseconds / animation->duration.count());
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