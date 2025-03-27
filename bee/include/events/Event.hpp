#pragma once
#include "common.hpp"

#define BIT(x) (1 << x)

#define BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

// used to define the type of the event
#define EVENT_CLASS_TYPE(type)                                                  \
    static EventType GetStaticType() { return EventType::type; }                \
    virtual EventType GetEventType() const override { return GetStaticType(); } \
    virtual const char* GetName() const override { return #type; }

// used to define the category of the event
#define EVENT_CLASS_CATEGORY(category) \
    virtual int GetCategoryFlags() const override { return category; }

namespace bee
{
enum class EventType
{
    None = 0,
    WindowClose,
    WindowResize,
    WindowFocus,
    WindowLostFocus,
    WindowMoved,
    KeyPressed,
    KeyReleased,
    KeyTyped,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled
};

// using Bit allows us to use them as flags, so multiple categories can be set
enum EventCategory
{
    None = 0,
    EventCategoryApplication = BIT(0),
    EventCategoryInput = BIT(1),
    EventCategoryKeyboard = BIT(2),
    EventCategoryMouse = BIT(3),
    EventCategoryMouseButton = BIT(4)
};

class Event
{
public:
    virtual EventType GetEventType() const = 0;
    virtual const char* GetName() const = 0;
    virtual int GetCategoryFlags() const = 0;
    virtual std::string ToString() const { return GetName(); }

    inline bool IsInCategory(EventCategory category) { return GetCategoryFlags() & category; }

    bool handled = false;
};

class EventDispatcher
{
public:
    EventDispatcher(Event& event) : m_event(event) {}

    template <typename T, typename F>
    bool Dispatch(const F& func)
    {
        if (m_event.GetEventType() == T::GetStaticType())
        {
            m_event.handled |= func(static_cast<T&>(m_event));
            return true;
        }
        return false;
    }

private:
    Event& m_event;
};
}  // namespace bee


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/