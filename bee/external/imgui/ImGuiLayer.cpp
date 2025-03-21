// #include "pch.h"
#include "imgui/ImGuiLayer.h"

#include "imgui_impl.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/implot.h"
#include "imgui/ImGuizmo.h"

#include "core.hpp"

#include "core/engine.hpp"
#include "core/device.hpp"
#include "core/fileio.hpp"

bee::ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer") {}

bee::ImGuiLayer::~ImGuiLayer() {}

void bee::ImGuiLayer::OnAttach() {}

void bee::ImGuiLayer::OnDetach()
{
    ImGui_Impl_Shutdown();
    ImGui::DestroyContext();
    ImPlot::DestroyContext();
}

void bee::ImGuiLayer::OnEngineInit()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform Windows

#ifdef BEE_PLATFORM_PC

    int screenWidth = bee::Engine.Device().GetWidth();
    int screenHeight = bee::Engine.Device().GetHeight();
    int baseWidth = 1920;
    int baseHeight = 1080;
    float scaleX = (float)screenWidth / baseWidth;
    float scaleY = (float)screenHeight / baseHeight;
    float scaler = std::min(scaleX, scaleY);

    float fontSize = roundf(18.0f * scaler);
    auto fontPath = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, "fonts/opensans/OpenSans-Bold.ttf");
    io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), fontSize);
    fontPath = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, "fonts/opensans/OpenSans-Regular.ttf");
    io.FontDefault = io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), fontSize);

    static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    ImFontConfig config;
    config.MergeMode = true;

    fontPath = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, "fonts/fa-regular-combined-400.ttf");
    io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), fontSize, &config, icon_ranges);
#endif

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Got color from ChatGPT
    {
        auto& colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};

        // Headers
        colors[ImGuiCol_Header] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_HeaderHovered] = ImVec4{0.93f, 0.45f, 0.13f, 1.0f};
        colors[ImGuiCol_HeaderActive] = ImVec4{0.7f, 0.34f, 0.1f, 1.0f};

        // Buttons
        colors[ImGuiCol_Button] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_ButtonHovered] = ImVec4{0.85f, 0.41f, 0.12f, 1.0f};  // Slightly darker hover color
        colors[ImGuiCol_ButtonActive] = ImVec4{0.7f, 0.34f, 0.1f, 1.0f};

        // Frame BG
        colors[ImGuiCol_FrameBg] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_FrameBgHovered] = ImVec4{0.85f, 0.41f, 0.12f, 1.0f};  // Slightly darker hover color
        colors[ImGuiCol_FrameBgActive] = ImVec4{0.7f, 0.34f, 0.1f, 1.0f};

        // Checkmarks (Checkboxes)
        colors[ImGuiCol_CheckMark] = ImVec4{0.67f, 0.33f, 0.09f, 1.0f};  // Orange checkmark

        // Sliders
        colors[ImGuiCol_SliderGrab] = ImVec4{0.93f, 0.45f, 0.13f, 1.0f};      // Orange slider
        colors[ImGuiCol_SliderGrabActive] = ImVec4{0.7f, 0.34f, 0.1f, 1.0f};  // Darker orange slider when active

        // Scrollbars
        colors[ImGuiCol_ScrollbarBg] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_ScrollbarGrab] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{0.85f, 0.41f, 0.12f, 1.0f};  // Slightly darker hover color
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{0.7f, 0.34f, 0.1f, 1.0f};     // Darker orange when active

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4{0.7f, 0.34f, 0.1f, 1.0f};
        colors[ImGuiCol_TabHovered] = ImVec4{0.85f, 0.41f, 0.12f, 1.0f};  // Slightly darker hover color
        colors[ImGuiCol_TabActive] = ImVec4{0.93f, 0.45f, 0.13f, 1.0f};
        colors[ImGuiCol_TabUnfocused] = ImVec4{0.5f, 0.25f, 0.08f, 1.0f};
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.7f, 0.34f, 0.1f, 1.0f};

        // Titles
        colors[ImGuiCol_TitleBg] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TitleBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Resize grip
        colors[ImGuiCol_ResizeGrip] = ImVec4{0.93f, 0.45f, 0.13f, 1.0f};
        colors[ImGuiCol_ResizeGripHovered] = ImVec4{0.85f, 0.41f, 0.12f, 1.0f};  // Slightly darker hover color
        colors[ImGuiCol_ResizeGripActive] = ImVec4{0.7f, 0.34f, 0.1f, 1.0f};

        // Text selection
        colors[ImGuiCol_TextSelectedBg] = ImVec4{0.93f, 0.45f, 0.13f, 0.35f};

        // Plot lines
        colors[ImGuiCol_PlotLines] = ImVec4{0.93f, 0.45f, 0.13f, 1.0f};
        colors[ImGuiCol_PlotLinesHovered] = ImVec4{0.7f, 0.34f, 0.1f, 1.0f};

        // Plot histograms
        colors[ImGuiCol_PlotHistogram] = ImVec4{0.93f, 0.45f, 0.13f, 1.0f};
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4{0.7f, 0.34f, 0.1f, 1.0f};
    }

    ImGui_Impl_Init();
}

void bee::ImGuiLayer::Begin()
{
    ImGui_Impl_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    if (bee::Engine.Device().IsCursorHidden())
    {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
    }
    else
    {
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    }

#ifdef EDITOR_MODE
    bool isPlayingGame = bee::Engine.IsPlaying();
    // If playing game, adjust several elements to a more consistent red tint
    if (isPlayingGame)
    {
        // Darker, more subdued red tint for WindowBg
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.05f, 0.05f, 1.0f));  // Darker window background
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.05f, 0.05f, 1.0f));

        // Tab headers with more contrast
        ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.4f, 0.15f, 0.15f, 1.0f));         // Slightly lighter red for inactive tabs
        ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));    // Lighter for hovered tabs
        ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.7f, 0.25f, 0.25f, 1.0f));   // Even lighter for active tabs
        ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(0.3f, 0.1f, 0.1f, 1.0f));  // Keep a darker tint for unfocused tabs

        // Optionally adjust title background and other elements for further distinction
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.3f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.4f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));

        m_pushedStyle = true;
    }
#endif
}

void bee::ImGuiLayer::End()
{
#ifdef EDITOR_MODE
    bool isPlayingGame = bee::Engine.IsPlaying();
    if ((isPlayingGame && m_pushedStyle) || (m_pushedStyle))
    {
        ImGui::PopStyleColor(12);
        m_pushedStyle = false;
    }
#else
    if (!ImGui::GetIO().WantCaptureMouse)
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
            ImGui::FocusWindow(nullptr);
#endif

    ImGui::Render();
    ImGui_Impl_RenderDrawData(ImGui::GetDrawData());

#ifdef BEE_PLATFORM_PC
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
#endif
}

// void bee::ImGuiLayer::OnEvent(events::Event& e) {
//	if (blockEvents) {
//		ImGuiIO& io = ImGui::GetIO();
//		e.handled |= e.IsInCategory(events::EventCategoryMouse) & io.WantCaptureMouse;
//		e.handled |= e.IsInCategory(events::EventCategoryKeyboard) & io.WantCaptureKeyboard;
//	}
// }
