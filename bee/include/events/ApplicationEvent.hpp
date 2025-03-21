#pragma once

#include "common.hpp"
#include "Event.hpp"

namespace bee
{
class WindowResizeEvent : public Event
{
public:
    WindowResizeEvent(unsigned int width, unsigned int height, bool minimized = false)
        : width(width), height(height), minimized(minimized)
    {
    }

    unsigned int GetWidth() const { return width; }
    unsigned int GetHeight() const { return height; }
    bool IsMinimized() const { return minimized; }

    std::string ToString() const override { return "WindowResizeEvent: " + std::to_string(width) + ", " + std::to_string(height); }

    EVENT_CLASS_TYPE(WindowResize);
    EVENT_CLASS_CATEGORY(EventCategoryApplication);

private:
    unsigned int width, height;
    bool minimized;
};

class WindowCloseEvent : public Event
{
public:
    WindowCloseEvent() {}

    EVENT_CLASS_TYPE(WindowClose);
    EVENT_CLASS_CATEGORY(EventCategoryApplication);
};
}  // namespace bee