#pragma once
#include "common.hpp"
#include "events/Event.hpp"

// #if defined(BEE_PLATFORM_PC) && defined(BEE_GRAPHICS_OPENGL)
// #include "platform/opengl/device_gl.hpp"
// #elif defined(BEE_PLATFORM_PROSPERO)
// #include "platform/prospero/core/device_prospero.hpp"
// #endif

namespace bee
{
class Device
{
public:
    using EventCallbackFn = std::function<void(bee::Event&)>;

    virtual ~Device() = default;

    virtual bool ShouldClose() = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual float GetMonitorUIScale() const = 0;

    virtual void* GetWindow() = 0;

    virtual void HideCursor(bool state) const = 0;

    virtual bool IsCursorHidden() const = 0;


    bool IsVSync() const { return m_data.vsync; }
    uint32_t GetWidth() const { return m_data.width; }
    uint32_t GetHeight() const { return m_data.height; }
    bool IsMinimized() const { return m_data.minimized; }
    bool IsFullscreen() const { return m_data.fullscreen; }
    const std::string& GetTitle() const { return m_data.title; }

    void SetEventCallback(const EventCallbackFn& callback) { m_data.eventCallback = callback; }

    static Device* Create();

protected:
    struct WindowData
    {
        std::string title;
        uint32_t width, height = 0;
        bool vsync = false;
        bool minimized = false;
        bool fullscreen = false;

        EventCallbackFn eventCallback = [](bee::Event&) {};
    };

    WindowData m_data;
};
}  // namespace bee