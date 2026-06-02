#pragma once

#include <algorithm>
#include <cmath>

namespace Animation {
float clamp01(float value);
float easeOutCubic(float t);
float easeInOutCubic(float t);
float easeOutBack(float t);
float lerp(float a, float b, float t);
float pulseAlpha(float timeSeconds, float speed);
}
