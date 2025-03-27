#include "core.hpp"

#include <GLFW/glfw3.h>
#include "imgui/imgui_impl_glfw.h"
#include "platform/pc/input_pc.hpp"

using namespace bee;

// class InputPc::Impl
//{
// };

enum KeyAction
{
    Release = 0,
    Press = 1,
    None = 2
};

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

constexpr int nr_keys = 350;
bool keys_down[nr_keys];
bool prev_keys_down[nr_keys];
KeyAction keys_action[nr_keys];

constexpr int nr_mousebuttons = 8;
bool mousebuttons_down[nr_mousebuttons];
bool prev_mousebuttons_down[nr_mousebuttons];
KeyAction mousebuttons_action[nr_mousebuttons];

constexpr int max_nr_gamepads = 4;
bool gamepad_connected[max_nr_gamepads];
GLFWgamepadstate gamepad_state[max_nr_gamepads];
GLFWgamepadstate prev_gamepad_state[max_nr_gamepads];

glm::vec2 mousepos;
glm::vec2 mousepos_prev;
float mousewheel = 0;

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

// NOLINTBEGIN(readability-convert-member-functions-to-static, misc-unused-parameters)
// disable lints because platforms have functions behaving differently

InputPc::InputPc() : Input() { Update(); }

bee::InputPc::~InputPc() {}

//InputPc::~InputPc()
//{
//    // auto* window = static_cast<GLFWwindow*>(Engine.Device().GetWindow());
//}

void InputPc::Update()
{
    // update keyboard key states
    for (int i = 0; i < nr_keys; ++i)
    {
        prev_keys_down[i] = keys_down[i];

        if (keys_action[i] == KeyAction::Press)
            keys_down[i] = true;
        else if (keys_action[i] == KeyAction::Release)
            keys_down[i] = false;

        keys_action[i] = KeyAction::None;
    }

    // update mouse button states
    for (int i = 0; i < nr_mousebuttons; ++i)
    {
        prev_mousebuttons_down[i] = mousebuttons_down[i];

        if (mousebuttons_action[i] == KeyAction::Press)
            mousebuttons_down[i] = true;
        else if (mousebuttons_action[i] == KeyAction::Release)
            mousebuttons_down[i] = false;

        mousebuttons_action[i] = KeyAction::None;
    }

    // update gamepad states
    for (int i = 0; i < max_nr_gamepads; ++i)
    {
        prev_gamepad_state[i] = gamepad_state[i];

        if (glfwJoystickPresent(i) && glfwJoystickIsGamepad(i))
            gamepad_connected[i] = static_cast<bool>(glfwGetGamepadState(i, &gamepad_state[i]));
    }

    mousepos_prev = mousepos;
    mousewheel = 0;
}

void bee::InputPc::OnEvent(Event& e)
{
    bee::EventDispatcher dispatcher(e);
    dispatcher.Dispatch<bee::KeyPressedEvent>(
        [](bee::KeyPressedEvent& e)
        {
            //if (io.WantCaptureKeyboard) return false;
            if (e.GetKeyCode() >= nr_keys) return false;
            keys_action[e.GetKeyCode()] = KeyAction::Press;
            return false;
        });

    dispatcher.Dispatch<bee::KeyReleasedEvent>(
        [](bee::KeyReleasedEvent& e)
        {
            if (e.GetKeyCode() >= nr_keys) return false;
            keys_action[e.GetKeyCode()] = KeyAction::Release;
            return false;
        });

    dispatcher.Dispatch<bee::MouseButtonPressedEvent>(
        [](bee::MouseButtonPressedEvent& e)
        {
            ImGuiIO& io = ImGui::GetIO();
            if (io.WantCaptureMouse) return false;
            mousebuttons_action[e.GetMouseButton()] = KeyAction::Press;
            return false;
        });

    dispatcher.Dispatch<bee::MouseButtonReleasedEvent>(
        [](bee::MouseButtonReleasedEvent& e)
        {
            mousebuttons_action[e.GetMouseButton()] = KeyAction::Release;
            return false;
        });

    dispatcher.Dispatch<bee::MouseMovedEvent>(
        [](bee::MouseMovedEvent& e)
        {
            mousepos.x = e.GetX();
            mousepos.y = e.GetY();
            return false;
        });

    dispatcher.Dispatch<bee::MouseScrolledEvent>(
        [](bee::MouseScrolledEvent& e)
        {
            //ImGuiIO& io = ImGui::GetIO();
            //if (io.WantCaptureMouse) return false;
            mousewheel = e.GetYOffset();
            return false;
        });
}

bool InputPc::IsGamepadAvailable(int gamepadID) const { return gamepad_connected[gamepadID]; }

float InputPc::GetGamepadAxis(int gamepadID, GamepadAxis axis) const
{
    if (!IsGamepadAvailable(gamepadID)) return 0.0;

    int a = static_cast<int>(axis);
    assert(a >= 0 && a <= GLFW_GAMEPAD_AXIS_LAST);
    return gamepad_state[gamepadID].axes[a];
}

float InputPc::GetGamepadAxisPrevious(int gamepadID, GamepadAxis axis) const
{
    if (!IsGamepadAvailable(gamepadID)) return 0.0;

    int a = static_cast<int>(axis);
    assert(a >= 0 && a <= GLFW_GAMEPAD_AXIS_LAST);
    return prev_gamepad_state[gamepadID].axes[a];
}

bool InputPc::GetGamepadButton(int gamepadID, GamepadButton button) const
{
    if (!IsGamepadAvailable(gamepadID)) return false;

    int b = static_cast<int>(button);
    assert(b >= 0 && b <= GLFW_GAMEPAD_BUTTON_LAST);
    return static_cast<bool>(gamepad_state[gamepadID].buttons[b]);
}

bool InputPc::GetGamepadButtonOnce(int gamepadID, GamepadButton button) const
{
    if (!IsGamepadAvailable(gamepadID)) return false;

    int b = static_cast<int>(button);

    assert(b >= 0 && b <= GLFW_GAMEPAD_BUTTON_LAST);
    return !static_cast<bool>(prev_gamepad_state[gamepadID].buttons[b]) &&
           static_cast<bool>(gamepad_state[gamepadID].buttons[b]);
}

bool InputPc::IsMouseAvailable() const { return true; }

bool InputPc::GetMouseButton(MouseButton button) const
{
    int b = static_cast<int>(button);
    return mousebuttons_down[b];
}

bool InputPc::GetMouseButtonOnce(MouseButton button) const
{
    int b = static_cast<int>(button);
    return mousebuttons_down[b] && !prev_mousebuttons_down[b];
}

glm::vec2 InputPc::GetMousePosition() const { return mousepos; }

glm::vec2 bee::InputPc::GetPreviousMousePosition() const { return mousepos_prev; }

float InputPc::GetMouseWheel() const { return mousewheel; }

bool InputPc::IsKeyboardAvailable() const { return true; }

bool InputPc::GetKeyboardKey(KeyboardKey key) const
{
    int k = static_cast<int>(key);
    assert(k >= GLFW_KEY_SPACE && k <= GLFW_KEY_LAST);
    return keys_down[k];
}

bool InputPc::GetKeyboardKeyOnce(KeyboardKey key) const
{
    int k = static_cast<int>(key);
    assert(k >= GLFW_KEY_SPACE && k <= GLFW_KEY_LAST);
    return keys_down[k] && !prev_keys_down[k];
}

// NOLINTEND(readability-convert-member-functions-to-static, misc-unused-parameters)



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/