#pragma once
#include "core/input.hpp"

namespace bee
{
class InputPc : public Input
{
public:
    bool IsGamepadAvailable(int gamepadID) const override;
    float GetGamepadAxis(int gamepadID, GamepadAxis axis) const override;
    float GetGamepadAxisPrevious(int gamepadID, GamepadAxis axis) const override;
    bool GetGamepadButton(int gamepadID, GamepadButton button) const override;
    bool GetGamepadButtonOnce(int gamepadID, GamepadButton button) const override;
    bool IsMouseAvailable() const override;
    bool GetMouseButton(MouseButton button) const override;
    bool GetMouseButtonOnce(MouseButton button) const override;
    glm::vec2 GetMousePosition() const override;
    glm::vec2 GetPreviousMousePosition() const override;
    float GetMouseWheel() const override;
    bool IsKeyboardAvailable() const override;
    bool GetKeyboardKey(KeyboardKey button) const override;
    bool GetKeyboardKeyOnce(KeyboardKey button) const override;

private:
    friend class Input;

    InputPc();
    ~InputPc();

    void Update() override;
    void OnEvent(Event& e) override;
};
}  // namespace bee