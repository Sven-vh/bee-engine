#pragma once

#include "common.hpp"
#include "Event.hpp"
#include "input/MouseCode.hpp"

namespace bee
{
class MouseMovedEvent : public Event
{
public:
    MouseMovedEvent(float x, float y) : x(x), y(y) {}

    float GetX() const { return x; }
    float GetY() const { return y; }

    std::string ToString() const override { return "MouseMovedEvent: " + std::to_string(x) + ", " + std::to_string(y); }

    EVENT_CLASS_TYPE(MouseMoved);
    EVENT_CLASS_CATEGORY(EventCategoryMouse);

private:
    float x, y;
};

class MouseScrolledEvent : public Event
{
public:
    MouseScrolledEvent(float xOffset, float yOffset) : xOffset(xOffset), yOffset(yOffset) {}

    float GetXOffset() const { return xOffset; }
    float GetYOffset() const { return yOffset; }

    std::string ToString() const override
    {
        return "MouseScrolledEvent: " + std::to_string(xOffset) + ", " + std::to_string(yOffset);
    }

    EVENT_CLASS_TYPE(MouseScrolled);
    EVENT_CLASS_CATEGORY(EventCategoryMouse);

private:
    float xOffset, yOffset;
};

class MouseButtonEvent : public Event
{
public:
    MouseCode GetMouseButton() const { return button; }
    float GetX() const { return xPos; }
    float GetY() const { return yPos; }

    EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton)
protected:
    MouseButtonEvent(const MouseCode button, float xPos, float yPos) : button(button), xPos(xPos), yPos(yPos) {}

    MouseCode button;
    float xPos, yPos;
};

class MouseButtonPressedEvent : public MouseButtonEvent
{
public:
    MouseButtonPressedEvent(const MouseCode button, float xPos, float yPos) : MouseButtonEvent(button, xPos, yPos) {}

    std::string ToString() const override { return "MouseButtonPressedEvent: " + std::to_string(button); }

    EVENT_CLASS_TYPE(MouseButtonPressed)
};

class MouseButtonReleasedEvent : public MouseButtonEvent
{
public:
    MouseButtonReleasedEvent(const MouseCode button, float xPos, float yPos) : MouseButtonEvent(button, xPos, yPos) {}

    std::string ToString() const override { return "MouseButtonReleasedEvent: " + std::to_string(button); }

    EVENT_CLASS_TYPE(MouseButtonReleased)
};
}  // namespace bee