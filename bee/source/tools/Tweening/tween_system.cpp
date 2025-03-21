#include "tools/Tweening/tween_system.hpp"

namespace bee::tween::internal
{
std::vector<TweenRef> tweens;
}

using namespace bee::tween::internal;

void bee::Tweener::Update(float dt)
{
    // for each tween in tweens
    for (auto it = tweens.begin(); it != tweens.end();)
    {
        (*it)->Update(dt);
        if ((*it)->IsFinished())
        {
            it = tweens.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void bee::Tweener::AddTween(const TweenRef& tween) { tweens.push_back(tween); }

void bee::Tweener::RemoveTween(const TweenRef& tween)
{
    tweens.erase(std::remove(tweens.begin(), tweens.end(), tween), tweens.end());
}
