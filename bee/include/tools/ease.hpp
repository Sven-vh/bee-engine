#pragma once

// This header file was created by Chatgpt. Could I have made it myself, yes. But just asking chatgpt to do it is faster and
// easier.

#include <cmath>
#include <functional>
#include <stdexcept>

namespace bee
{

enum class EaseType
{
    Linear,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
    EaseInQuart,
    EaseOutQuart,
    EaseInOutQuart,
    EaseInQuint,
    EaseOutQuint,
    EaseInOutQuint,
    Count
};

template <typename T>
inline T lerp(const T& a, const T& b, float t)
{
    return a + t * (b - a);
}

// https://easings.net/
inline std::function<float(float)> getEaseFunction(EaseType easeType)
{
    switch (easeType)
    {
        case EaseType::Linear:
            return [](float t) { return t; };
        case EaseType::EaseInQuad:
            return [](float t) { return t * t; };
        case EaseType::EaseOutQuad:
            return [](float t) { return t * (2 - t); };
        case EaseType::EaseInOutQuad:
            return [](float t) { return t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t; };
        case EaseType::EaseInCubic:
            return [](float t) { return t * t * t; };
        case EaseType::EaseOutCubic:
            return [](float t)
            {
                t -= 1;
                return t * t * t + 1;
            };
        case EaseType::EaseInOutCubic:
            return [](float t) { return t < 0.5f ? 4 * t * t * t : (t - 1) * (2 * t - 2) * (2 * t - 2) + 1; };
        case EaseType::EaseInQuart:
            return [](float t) { return t * t * t * t; };
        case EaseType::EaseOutQuart:
            return [](float t)
            {
                t -= 1;
                return 1 - t * t * t * t;
            };  // Fixes modification
        case EaseType::EaseInOutQuart:
            return [](float t)
            {
                if (t < 0.5f)
                {
                    return 8 * t * t * t * t;
                }
                else
                {
                    t -= 1;  // Decrement `t` before using it
                    return 1 - 8 * t * t * t * t;
                }
            };
        case EaseType::EaseInQuint:
            return [](float t) { return t * t * t * t * t; };
        case EaseType::EaseOutQuint:
            return [](float t)
            {
                t -= 1;
                return 1 + t * t * t * t * t;
            };  // Fixes modification
        case EaseType::EaseInOutQuint:
            return [](float t)
            { return t < 0.5f ? 16 * t * t * t * t * t : (t -= 1, 1 + 16 * t * t * t * t * t); };  // Fixes modification
        default:
            throw std::invalid_argument("Unsupported m_ease type.");
    }
}

template <typename T>
inline T easeLerp(const T& a, const T& b, float t, EaseType easeType)
{
    auto easeFunc = getEaseFunction(easeType);
    float easedT = easeFunc(t);
    return lerp<T>(a, b, easedT);
}

}  // namespace bee
