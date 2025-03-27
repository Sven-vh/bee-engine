#pragma once
#include "common.hpp"
#include "Event.hpp"
#include "input/KeyCode.hpp"

namespace bee
{
class KeyEvent : public Event
{
public:
    KeyCode GetKeyCode() const { return keyCode; }

    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
protected:
    KeyEvent(KeyCode keyCode) : keyCode(keyCode) {}
    KeyCode keyCode;
};

// ====== Key Pressed Event ======
class KeyPressedEvent : public KeyEvent
{
public:
    KeyPressedEvent(int keyCode, bool isRepeat = false) : KeyEvent(keyCode), isRepeat(isRepeat) {}

    bool IsRepeat() const { return isRepeat; }

    std::string ToString() const override { return "KeyPressedEvent: " + std::to_string(keyCode); }

    EVENT_CLASS_TYPE(KeyPressed);

private:
    bool isRepeat;
};

// ====== Key Released Event ======
class KeyReleasedEvent : public KeyEvent
{
public:
    KeyReleasedEvent(int keyCode) : KeyEvent(keyCode) {}

    std::string ToString() const override { return "KeyReleasedEvent: " + std::to_string(keyCode); }

    EVENT_CLASS_TYPE(KeyReleased);
};

// ====== Key Typed Event ======
class KeyTypedEvent : public KeyEvent
{
public:
    KeyTypedEvent(int keyCode) : KeyEvent(keyCode) {}

    std::string ToString() const override { return "KeyTypedEvent: " + std::to_string(keyCode); }

    EVENT_CLASS_TYPE(KeyTyped);
};
}  // namespace bee


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/