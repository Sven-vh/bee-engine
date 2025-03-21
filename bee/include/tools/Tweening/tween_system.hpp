#pragma once
#include "core.hpp"
#include "tools/Tweening/tween.hpp"

#define CREATE_REF_TWEEN(T, address, endValue, duration) \
    std::make_shared<bee::RefTween<T>>(std::make_shared<T>(address), endValue, duration)

#define CREATE_TWEEN(T, startValue, endValue, duration) \
    std::make_shared<bee::Tween<T>>(startValue, endValue, duration)


namespace bee
{
using TweenRef = std::shared_ptr<bee::ITween>;
class Tweener
{
public:
    static void Update(float dt);

    static void AddTween(const TweenRef& tween);
    static void RemoveTween(const TweenRef& tween);
};
}  // namespace bee