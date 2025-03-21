#pragma once
#include <functional>
#include <memory>
#include "common.hpp"
#include "tools/ease.hpp"

namespace bee
{

class ITween
{
public:
    virtual ~ITween() = default;
    virtual void Update(const float deltaTime) = 0;
    virtual bool IsFinished() const = 0;
};

#pragma region RefTween
template <typename T>
class RefTween : public ITween
{
public:
    RefTween();
    RefTween(std::shared_ptr<T> adress, T endValue, const float duration);
    ~RefTween();

    RefTween& OnUpdate(const std::function<void(T, float)> _onUpdate)
    {
        this->m_onUpdate = _onUpdate;
        return *this;
    }

    RefTween& OnFinish(const std::function<void(T)> _onComplete)
    {
        this->m_onComplete = _onComplete;
        return *this;
    }

    RefTween& SetEase(bee::EaseType ease)
    {
        this->m_ease = ease;
        return *this;
    }

protected:
    void Update(const float deltaTime) override;
    bool IsFinished() const override;

private:
    std::shared_ptr<T> m_adress;  // Now we directly use shared_ptr
    T m_startValue;
    T m_endValue;
    bee::EaseType m_ease;
    float m_duration;
    float m_elapsed;

    std::function<void(T, float)> m_onUpdate;
    std::function<void(T)> m_onComplete;
};

template <typename T>
inline RefTween<T>::RefTween() : m_duration(0), m_elapsed(0), m_ease(bee::EaseType::Linear)
{
}

template <typename T>
inline RefTween<T>::RefTween(std::shared_ptr<T> adress, T endValue, const float duration)
    : m_adress(adress),
      m_endValue(endValue),
      m_duration(duration),
      m_elapsed(0),
      m_ease(bee::EaseType::Linear),
      m_onUpdate([](T, float) {}),
      m_onComplete([](T) {})
{
    // Initialize startValue directly from the shared_ptr
    m_startValue = *m_adress;
}

template <typename T>
inline RefTween<T>::~RefTween()
{
}

template <typename T>
inline void RefTween<T>::Update(const float deltaTime)
{
    m_elapsed += deltaTime;
    float t = m_elapsed / m_duration;
    t = std::min(t, 1.0f);
    *m_adress = bee::easeLerp(m_startValue, m_endValue, t, m_ease);
    m_onUpdate(*m_adress, t);

    if (IsFinished())
    {
        *m_adress = m_endValue;
        m_onComplete(*m_adress);
    }
}

template <typename T>
inline bool RefTween<T>::IsFinished() const
{
    return m_elapsed >= m_duration;
}
#pragma endregion

#pragma region Tween
template <typename T>
class Tween : public ITween
{
public:
    Tween();
    Tween(T startValue, T endValue, const float duration);
    ~Tween();

    Tween& OnUpdate(const std::function<void(T, float)> _onUpdate)
    {
        this->m_onUpdate = _onUpdate;
        return *this;
    }

    Tween& OnFinish(const std::function<void(T)> _onComplete)
    {
        this->m_onComplete = _onComplete;
        return *this;
    }

    Tween& SetEase(bee::EaseType ease)
    {
        this->m_ease = ease;
        return *this;
    }

    Tween& from(T startValue)
    {
        this->m_startValue = startValue;
        return *this;
    }

    Tween& to(T endValue)
    {
        this->m_endValue = endValue;
        return *this;
    }

    Tween& duration(float duration)
    {
        this->m_duration = duration;
        return *this;
    }

protected:
    void Update(const float deltaTime) override;
    bool IsFinished() const override;

private:
    T m_startValue;  // Store start and end values directly
    T m_endValue;
    bee::EaseType m_ease;
    float m_duration;
    float m_elapsed;

    std::function<void(T, float)> m_onUpdate;  // Callback for updates
    std::function<void(T)> m_onComplete;       // Callback for when tween is complete
};

template <typename T>
inline Tween<T>::Tween() : m_startValue(T()), m_endValue(T()), m_duration(0), m_elapsed(0), m_ease(bee::EaseType::Linear)
{
}

template <typename T>
inline Tween<T>::Tween(T startValue, T endValue, const float duration)
    : m_startValue(startValue),
      m_endValue(endValue),
      m_ease(bee::EaseType::Linear),
      m_duration(duration),
      m_elapsed(0),
      m_onUpdate([](T, float) {}),
      m_onComplete([](T) {})
{
}

template <typename T>
inline Tween<T>::~Tween()
{
}

template <typename T>
inline void Tween<T>::Update(const float deltaTime)
{
    m_elapsed += deltaTime;
    float t = m_elapsed / m_duration;
    t = std::min(t, 1.0f);

    // Apply easing function to interpolate between start and end values
    T currentValue = bee::easeLerp(m_startValue, m_endValue, t, m_ease);

    // Call the onUpdate callback with the current value and progress
    m_onUpdate(currentValue, t);

    // If the tween is finished, invoke onComplete callback
    if (IsFinished())
    {
        m_onComplete(m_endValue);
    }
}

template <typename T>
inline bool Tween<T>::IsFinished() const
{
    return m_elapsed >= m_duration;
}

#pragma endregion

}  // namespace bee