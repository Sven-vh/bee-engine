#pragma once
#include "events/Event.hpp"
#include "core/device.hpp"

struct GLFWwindow;
struct GLFWmonitor;

namespace bee
{

class OpenGLDevice : public Device
{
public:
    OpenGLDevice();
    ~OpenGLDevice();

    bool ShouldClose() override;
    void BeginFrame() override;
    void EndFrame() override;
    float GetMonitorUIScale() const override;
    void* GetWindow() override;

    void HideCursor(bool state) const override;

    bool IsCursorHidden() const override;

    static void ResizeWindow(GLFWwindow* window, int width, int height);

private:
    friend class EngineClass;

    GLFWwindow* m_window = nullptr;
    GLFWmonitor* m_monitor = nullptr;
};

}  // namespace bee



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/