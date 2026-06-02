#include "Animation.h"

namespace Animation {
float clamp01(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

float easeOutCubic(float t) {
    t = clamp01(t);
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

float easeInOutCubic(float t) {
    t = clamp01(t);
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;
}

float easeOutBack(float t) {
    t = clamp01(t);
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
}

float lerp(float a, float b, float t) {
    return a + (b - a) * clamp01(t);
}

float pulseAlpha(float timeSeconds, float speed) {
    return (std::sin(timeSeconds * speed) + 1.0f) * 0.5f;
}
}
