#include "imgui.h"
#include "imgui_impl.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "core/engine.hpp"
#include "core/device.hpp"
#include "platform/opengl/open_gl.hpp"

bool ImGui_Impl_Init()
{
    const auto window = static_cast<GLFWwindow*>(bee::Engine.Device().GetWindow());
    const bool opengl = ImGui_ImplOpenGL3_Init();
    const bool glfw = ImGui_ImplGlfw_InitForOpenGL(window, true);
    return glfw && opengl;
}

void ImGui_Impl_Shutdown() { ImGui_ImplGlfw_Shutdown(); }

void ImGui_Impl_NewFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
}

void ImGui_Impl_RenderDrawData(ImDrawData*)
{
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //glClearColor(0.3f, 0.2f, 0.3f, 1.0f);
    //glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}