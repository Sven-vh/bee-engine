#pragma once
#include "rendering/PerspectiveCamera.hpp"
#include "core/LayerStack.hpp"
// #include "common.hpp"
// #include "imgui/ImGuiLayer.h"
// #include "core/fileio.hpp"
// #include "core/input.hpp"
// #include "core/audio.hpp"
// #include "editor/EditorLayer.hpp"

struct EngineSettings
{
    // Should be set by the application, relative to the .sln/$(WorkingDir).
    std::filesystem::path projectPath;

    bool interpolateParticles = false;
    float timeScale = 1.0f;
    float fixedUpdateRate = 60.0f;
};

namespace bee
{

class FileIO;
class Device;
class Input;
class Audio;
class ImGuiLayer;
class EditorLayer;
class WindowCloseEvent;
class KeyPressedEvent;
class WindowResizeEvent;

class EngineClass
{
public:
    void Initialize(const EngineSettings& settings);
    void Shutdown();
    void Run();

    void PushAppLayer(Layer* layer);
    void PushAppOverlay(Layer* overlay);

    template <class derived>
    derived* GetAppLayer()
    {
        return m_applicationLayerStack.GetLayer<derived>();
    }

    FileIO& FileIO() { return *m_fileIO; }
    Device& Device() { return *m_device; }
    Input& Input() { return *m_input; }
    Audio& Audio() { return *m_audio; }
    entt::registry& Registry() { return m_registry; }
    entt::entity EditorCamera() { return m_editorCamera; }
    entt::entity MainCamera() { return m_mainCamera; }
    EngineSettings& Settings() { return m_settings; }

    bool IsPaused() const { return m_paused; }
    void Pause() { m_paused = true; }
    void Resume() { m_paused = false; }

    float GetGameTime() const { return m_gameTime; }
    float GetRealTime() const { return m_realTime; }

    bool IsPlaying() const { return m_playing; }

    void SetCamera(entt::entity camera) { m_mainCamera = camera; }

private:
    bool m_running = true;
    bool m_paused = false;
    bool m_playing = false;
    EngineSettings m_settings;
    
    float m_gameTime = 0.0f;
    float m_realTime = 0.0f;

    bee::FileIO* m_fileIO = nullptr;
    bee::Device* m_device = nullptr;
    bee::Input* m_input = nullptr;
    bee::Audio* m_audio = nullptr;

    bee::LayerStack m_applicationLayerStack;
    entt::registry m_registry;  // It works here
    bee::ImGuiLayer* m_imguiLayer;
    entt::entity m_editorCamera = entt::null;
    entt::entity m_mainCamera = entt::null;

#ifdef EDITOR_MODE
    bee::LayerStack m_editorLayerStack;
    bee::EditorLayer* m_editorLayer;
#endif
    // entt::registry m_registry; // it crashes if I put it here
private:
    friend class EditorLayer;
    void Update(float deltaTime);
    void FixedUpdate(float deltaTime);
    void Draw();
    void RenderCameras();
    void RenderUICameras();
    void ImGuiRender();

#ifdef EDITOR_MODE
    void PushEditorLayer(Layer* layer);
    void PushEditorOverlay(Layer* overlay);
#endif

    void OnEvent(Event& e);

    void StartApplication();
    void StopApplication();

    bool OnWindowClose(WindowCloseEvent& e);
    bool OnKeyPressed(KeyPressedEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);
};

extern EngineClass Engine;

}  // namespace bee