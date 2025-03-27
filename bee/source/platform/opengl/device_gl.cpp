#include <cassert>
#define GLFW_INCLUDE_NONE
#include "platform/opengl/open_gl.hpp"
#include "platform/opengl/device_gl.hpp"

#include "core.hpp"

using namespace bee;

static void ErrorCallback(int, const char* description) { fputs(description, stderr); }

void LogOpenGLVersionInfo()
{
    const auto* const vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const auto* const renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const auto* const version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const auto* const shaderVersion = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));

    Log::Info("OpenGL Vendor {}", vendor);
    Log::Info("OpenGL Renderer {}", renderer);
    Log::Info("OpenGL Version {}", version);
    Log::Info("OpenGL Shader Version {}", shaderVersion);
}

bee::OpenGLDevice::OpenGLDevice()
{
    if (!glfwInit())
    {
        Log::Critical("GLFW init failed");
        assert(false);
        exit(EXIT_FAILURE);
    }

    Log::Info("GLFW version {}.{}.{}", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);

    glfwSetErrorCallback(ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if defined(DEBUG)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_FALSE);
#endif

#if defined(BEE_INSPECTOR)
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#endif

    m_monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(m_monitor);

    auto maxScreenWidth = mode->width;
    auto maxScreenHeight = mode->height;

    m_data.width = 1920;
    m_data.height = 1080;

    m_data.fullscreen = false;

    if (m_data.fullscreen)
    {
        m_data.width = maxScreenWidth;
        m_data.height = maxScreenHeight;
        m_window = glfwCreateWindow(m_data.width, m_data.height, "BEE", m_monitor, nullptr);
    }
    else
    {
        m_window = glfwCreateWindow(m_data.width, m_data.height, "BEE", nullptr, nullptr);
    }

    if (!m_window)
    {
        Log::Critical("GLFW window could not be created");
        glfwTerminate();
        assert(false);
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, &m_data);
    glfwMaximizeWindow(m_window);

    if (!m_data.vsync) glfwSwapInterval(0);

    int major = glfwGetWindowAttrib(m_window, GLFW_CONTEXT_VERSION_MAJOR);
    int minor = glfwGetWindowAttrib(m_window, GLFW_CONTEXT_VERSION_MINOR);
    int revision = glfwGetWindowAttrib(m_window, GLFW_CONTEXT_REVISION);
    Log::Info("GLFW OpenGL context version {}.{}.{}", major, minor, revision);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        Log::Critical("GLAD failed to initialize OpenGL context");
        assert(false);
        exit(EXIT_FAILURE);
    }

    LogOpenGLVersionInfo();
    InitDebugMessages();

    // glfw callbacks using lambda
    // Set framebuffer size callback to adjust viewport and update width and height
    glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height) { ResizeWindow(window, width, height); });

    // Set window close callback
    glfwSetWindowCloseCallback(m_window,
                               [](GLFWwindow* window)
                               {
                                   auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                                   WindowCloseEvent event;
                                   data.eventCallback(event);
                               });

    glfwSetKeyCallback(m_window,
                       [](GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
                       {
                           auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                           switch (action)
                           {
                               case GLFW_PRESS:
                               {
                                   KeyPressedEvent event(key, 0);
                                   data.eventCallback(event);
                                   break;
                               }
                               case GLFW_RELEASE:
                               {
                                   KeyReleasedEvent event(key);
                                   data.eventCallback(event);
                                   break;
                               }
                               case GLFW_REPEAT:
                               {
                                   KeyPressedEvent event(key, true);
                                   data.eventCallback(event);
                                   break;
                               }
                           }
                       });

    glfwSetMouseButtonCallback(
        m_window,
        [](GLFWwindow* window, int button, int action, int /*mods*/)
        {
            auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
            double xpos = 0.0, ypos = 0.0;
            glfwGetCursorPos(window, &xpos, &ypos);
            switch (action)
            {
                case GLFW_PRESS:
                {
                    MouseButtonPressedEvent event(button, static_cast<float>(xpos), static_cast<float>(ypos));
                    data.eventCallback(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    MouseButtonReleasedEvent event(button, static_cast<float>(xpos), static_cast<float>(ypos));
                    data.eventCallback(event);
                    break;
                }
            }
        });

    glfwSetCursorPosCallback(m_window,
                             [](GLFWwindow* window, double xpos, double ypos)
                             {
                                 auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                                 MouseMovedEvent event(static_cast<float>(xpos), static_cast<float>(ypos));
                                 data.eventCallback(event);
                             });

    glfwSetScrollCallback(m_window,
                          [](GLFWwindow* window, double xoffset, double yoffset)
                          {
                              auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                              MouseScrolledEvent event(static_cast<float>(xoffset), static_cast<float>(yoffset));
                              data.eventCallback(event);
                          });

    glfwSetCharCallback(m_window,
                        [](GLFWwindow* window, unsigned int keycode)
                        {
                            auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                            KeyTypedEvent event(keycode);
                            data.eventCallback(event);
                        });

    // resize window to trigger the callback
    int width = 0, height = 0;
    glfwGetWindowSize(m_window, &width, &height);
    ResizeWindow(m_window, width, height);
}

void bee::OpenGLDevice::ResizeWindow(GLFWwindow* window, int width, int height)
{
    auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
    if (data.width == (uint32_t)width && data.height == (uint32_t)height) return;
    data.width = width;
    data.height = height;
    data.minimized = width == 0 || height == 0;

    WindowResizeEvent event(width, height);
    data.eventCallback(event);

    glViewport(0, 0, width, height);
}

bee::OpenGLDevice::~OpenGLDevice() { glfwTerminate(); }

bool bee::OpenGLDevice::ShouldClose() { return false; }

void bee::OpenGLDevice::BeginFrame()
{
    glClearColor(0.93f, 0.45f, 0.13f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void bee::OpenGLDevice::EndFrame()
{
    glfwPollEvents();
    glfwSwapBuffers(m_window);
}

float bee::OpenGLDevice::GetMonitorUIScale() const { return 1.0f; }

void* bee::OpenGLDevice::GetWindow()
{
    // return the window
    return m_window;
}

void bee::OpenGLDevice::HideCursor(bool state) const
{
    if (state)
    {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        bee::Log::Info("Mouse is Locked, press F1 to unlock it.");
    }
    else
    {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

bool bee::OpenGLDevice::IsCursorHidden() const { return glfwGetInputMode(m_window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED; }



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/