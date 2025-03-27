#include "core/engine.hpp"
#include "core.hpp"
#include "imgui/ImGuiLayer.h"
#include "editor/EditorLayer.hpp"
#include "managers/particle_manager.hpp"

using namespace bee;

// Make the engine a global variable on free store memory.
bee::EngineClass bee::Engine;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void EngineClass::Initialize(const EngineSettings& settings)
{
    m_settings = settings;
    Log::Initialize();
    m_fileIO = new bee::FileIO();
    m_device = bee::Device::Create();
    m_input = bee::Input::Create();
    m_audio = new bee::Audio();
    m_registry = entt::registry();

    // Create render configuration
    xsr::render_configuration render_config;
    render_config.enable_shadows = true;
    render_config.texture_filter = xsr::render_configuration::texture_filtering::nearest;
    bool success = xsr::initialize(render_config);
    assert(success);

    m_imguiLayer = new ImGuiLayer();
#ifdef EDITOR_MODE
    PushEditorOverlay(m_imguiLayer);
#else
    PushAppOverlay(m_imguiLayer);
#endif  // EDITOR_MODE

#ifdef EDITOR_MODE
    int width = bee::Engine.Device().GetWidth();
    int height = bee::Engine.Device().GetHeight();
    m_editorCamera = bee::ecs::CreateEmpty();
    m_registry.emplace<bee::Transform>(m_editorCamera, glm::vec3(0.0f, 1.0f, 3.0f), glm::vec3(0.0f), glm::vec3(1.0f));
    m_registry.emplace<bee::Camera>(m_editorCamera, 45.0f, (float)width / (float)height, 0.1f, 100.0f);
    m_registry.emplace<bee::EditorComponent>(m_editorCamera);
    // m_mainCamera = m_editorCamera;

    m_editorLayer = new EditorLayer();
    PushEditorOverlay(m_editorLayer);
#endif
    m_device->SetEventCallback(BIND_EVENT_FN(EngineClass::OnEvent));

    RenderManager::Initialize();
    ComponentManager::RegisterComponents();
}

void EngineClass::Shutdown()
{
#ifdef EDITOR_MODE
    for (auto& layer : m_editorLayerStack)
    {
        if (!layer->loaded) continue;
        layer->OnDetach();
    }
#endif
    for (auto& layer : m_applicationLayerStack)
    {
        if (!layer->loaded) continue;
        layer->OnDetach();
    }

    delete m_input;
    delete m_audio;
    delete m_device;
    delete m_fileIO;
    xsr::shutdown();
}

void bee::EngineClass::OnEvent(Event& e)
{
    m_input->OnEvent(e);

#ifdef EDITOR_MODE
    for (Layer* layer : m_editorLayerStack)
    {
        if (!layer->loaded) continue;
        layer->OnEvent(e);
        if (e.handled) break;
    }
#endif

    for (Layer* layer : m_applicationLayerStack)
    {
        if (!layer->loaded) continue;
        layer->OnEvent(e);
        if (e.handled) break;
    }

    bee::EventDispatcher dispatcher(e);
    dispatcher.Dispatch<bee::WindowCloseEvent>(BIND_EVENT_FN(EngineClass::OnWindowClose));
    dispatcher.Dispatch<bee::KeyPressedEvent>(BIND_EVENT_FN(EngineClass::OnKeyPressed));
    dispatcher.Dispatch<bee::WindowResizeEvent>(BIND_EVENT_FN(EngineClass::OnWindowResize));
}

void bee::EngineClass::StartApplication()
{
    // loop through all layers and call OnAttach
    try
    {
        for (Layer* layer : m_applicationLayerStack)
        {
            if (layer->loaded) continue;
            layer->OnAttach();
            layer->loaded = true;
        }

        m_playing = true;

        if (m_mainCamera == entt::null)
        {
            bee::Log::Error("Main camera not set. Trying to find one...");
            auto view = m_registry.view<Camera>();

            for (auto entity : view)
            {
                if (m_registry.all_of<Canvas>(entity)) continue;
                m_mainCamera = entity;
                break;
            }
        }
    }
    catch (const std::exception& e)
    {
        bee::Log::Error("Error in layer On Attach: {}", e.what());
        StopApplication();
    }
}

void bee::EngineClass::StopApplication()
{
    // loop through all layers and call OnDetach
    for (Layer* layer : m_applicationLayerStack)
    {
        if (!layer->loaded) continue;
        layer->OnDetach();
        layer->loaded = false;
    }
    m_playing = false;

    bee::ecs::UnloadScene();

    m_device->HideCursor(false);
}

void EngineClass::Run()
{
    auto previousTime = std::chrono::high_resolution_clock::now();
    float accumulator = 0.0f;

#ifdef EDITOR_MODE
    for (Layer* layer : m_editorLayerStack)
    {
        layer->OnEngineInit();
    }
#endif
    for (Layer* layer : m_applicationLayerStack)
    {
        layer->OnEngineInit();
    }

#ifndef EDITOR_MODE
    StartApplication();
#endif

#ifdef EDITOR_MODE
    Camera& camera = m_registry.get<Camera>(m_editorCamera);
    camera.SetAspectRatio((float)m_device->GetWidth() / (float)m_device->GetHeight());
#else
    auto view = m_registry.view<Camera>();
    for (auto entity : view)
    {
        Camera& camera = m_registry.get<Camera>(entity);
        camera.SetAspectRatio((float)m_device->GetWidth() / (float)m_device->GetHeight());
    }
#endif

    while (m_running)
    {
        const float fixedDeltaTimeMS = 1.0f / m_settings.fixedUpdateRate;

        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsed = currentTime - previousTime;
        previousTime = currentTime;
        float dt = (float)std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() / 1000000.0f;
        m_realTime += dt;
        dt *= m_settings.timeScale;
        dt = std::min(dt, 0.1f);
        m_gameTime += dt;
        m_device->BeginFrame();

#ifdef EDITOR_MODE
        for (Layer* layer : m_editorLayerStack)
        {
            if (!layer->loaded) continue;
            layer->OnUpdate(dt);
        }
#endif

        if (!m_paused)
        {
            Update(dt);
            accumulator += dt;

            PROFILE_SECTION("Fixed Loop");
            while (accumulator >= fixedDeltaTimeMS)
            {
                FixedUpdate(fixedDeltaTimeMS);
                accumulator -= fixedDeltaTimeMS;
            }
        }
        // Interpolate particles to the "next" frame
        if (!m_paused && m_settings.interpolateParticles) ParticleManager::FixedUpdate(accumulator);

        Draw();

        // Interpolate particles back so that they are in the correct position for the next frame
        if (!m_paused && m_settings.interpolateParticles) ParticleManager::FixedUpdate(-accumulator);

        m_imguiLayer->Begin();
        ImGuiRender();
        m_imguiLayer->End();

        m_input->Update();
        m_device->EndFrame();
    }

    StopApplication();
}

void bee::EngineClass::Update(float deltaTime)
{
    try
    {
        for (Layer* layer : m_applicationLayerStack)
        {
            if (!layer->loaded) continue;
            layer->OnUpdate(deltaTime);
        }
    }
    catch (const std::exception& e)
    {
        bee::Log::Error("Error in layer update: {}", e.what());
        StopApplication();
    }

    ParticleManager::Update(deltaTime);
    Tweener::Update(deltaTime);
}

void bee::EngineClass::FixedUpdate(float fixedDeltaTime)
{
#ifdef EDITOR_MODE
    for (Layer* layer : m_editorLayerStack)
    {
        if (!layer->loaded) continue;
        layer->OnFixedUpdate(fixedDeltaTime);
    }
#endif
    try
    {
        for (Layer* layer : m_applicationLayerStack)
        {
            if (!layer->loaded) continue;
            layer->OnFixedUpdate(fixedDeltaTime);  // Pass fixed time step in milliseconds
        }
    }
    catch (const std::exception& e)
    {
        bee::Log::Error("Error in layer fixed update: {}", e.what());
        StopApplication();
    }

    ParticleManager::FixedUpdate(fixedDeltaTime);
}

void bee::EngineClass::Draw()
{
#ifdef EDITOR_MODE
    for (Layer* layer : m_editorLayerStack)
    {
        if (!layer->loaded) continue;
        layer->OnRender();
    }
#endif  // EDITOR_MODE

    try
    {
        for (Layer* layer : m_applicationLayerStack)
        {
            if (!layer->loaded) continue;
            layer->OnRender();
        }
    }
    catch (const std::exception& e)
    {
        bee::Log::Error("Error in layer render: {}", e.what());
        StopApplication();
    }
    ParticleManager::Draw();

    RenderManager::SubmitRenderables(m_registry);
    // TODO: Make billboards work with multiple cameras, probably do the billboarding in the shader
#ifdef EDITOR_MODE
    RenderManager::SubmitBillboards(m_playing ? m_mainCamera : m_editorCamera, m_registry);
#else
    RenderManager::SubmitBillboards(m_mainCamera, m_registry);
#endif
    RenderManager::SubmitLights(m_registry);
    RenderManager::SubmitSceneData(m_registry);

#ifdef EDITOR_MODE

    if (m_playing)
    {
        m_editorLayer->Begin(m_mainCamera);
        RenderManager::Render(m_mainCamera, m_registry);
        m_editorLayer->End(m_mainCamera);

        m_editorLayer->Begin(m_editorCamera);
        RenderManager::Render(m_editorCamera, m_registry);
        m_editorLayer->End(m_editorCamera);
    }
    else
    {
        RenderCameras();
    }
#else
    // RenderManager::Render(m_mainCamera, m_registry);
    RenderCameras();
#endif

    RenderManager::ClearEntries();

    RenderManager::SubmitUIRenderables(m_registry);

    RenderUICameras();

    RenderManager::ClearEntries();
}

void bee::EngineClass::RenderCameras()
{
    auto cameras = m_registry.view<Camera>();
    for (auto camera : cameras)
    {
        Camera& cameraComponent = cameras.get<Camera>(camera);

        if (!cameraComponent.render) continue;

        if (m_registry.all_of<Canvas>(camera)) continue;

#ifdef EDITOR_MODE
        m_editorLayer->Begin(camera);
        RenderManager::Render(camera, m_registry);
        m_editorLayer->End(camera);
#else
        RenderManager::Render(camera, m_registry);
#endif
    }
}

void bee::EngineClass::RenderUICameras()
{
    auto cameras = m_registry.view<Camera>();
    for (auto camera : cameras)
    {
        if (auto* canvas = m_registry.try_get<Canvas>(camera))
        {
#ifdef EDITOR_MODE
            if (m_playing)
            {
                m_editorLayer->Begin(m_mainCamera);
                Camera& uiCamera = m_registry.get<Camera>(camera);
                uiCamera.SetAspectRatio(m_registry.get<Camera>(m_mainCamera).aspectRatio);
                RenderManager::RenderUI(camera, m_registry);
                m_editorLayer->End(m_mainCamera);
            }
            else
            {
                m_editorLayer->Begin(camera);
                xsr::clear_screen();
                RenderManager::RenderUI(camera, m_registry);
                m_editorLayer->End(camera);
            }
#else
            RenderManager::RenderUI(camera, m_registry);
#endif
        }
    }
}

void bee::EngineClass::ImGuiRender()
{
    PROFILE_FUNCTION();

#ifdef EDITOR_MODE
    for (Layer* layer : m_editorLayerStack)
    {
        if (!layer->loaded) continue;
        layer->OnImGuiRender();
    }
#endif

    try
    {
        for (Layer* layer : m_applicationLayerStack)
        {
            if (!layer->loaded) continue;
            layer->OnImGuiRender();
        }
    }
    catch (const std::exception& e)
    {
        bee::Log::Error("Error in layer ImGui render: {}", e.what());
        StopApplication();
    }
}

void bee::EngineClass::PushAppLayer(Layer* layer) { m_applicationLayerStack.PushLayer(layer); }

void bee::EngineClass::PushAppOverlay(Layer* overlay) { m_applicationLayerStack.PushOverlay(overlay); }

#ifdef EDITOR_MODE
void bee::EngineClass::PushEditorLayer(Layer* layer)
{
    m_editorLayerStack.PushLayer(layer);
    layer->OnAttach();
    layer->loaded = true;
}

void bee::EngineClass::PushEditorOverlay(Layer* overlay)
{
    m_editorLayerStack.PushOverlay(overlay);
    overlay->OnAttach();
    overlay->loaded = true;
}
#endif

bool bee::EngineClass::OnWindowClose(WindowCloseEvent&)
{
    m_running = false;
    return true;
}

bool bee::EngineClass::OnKeyPressed(KeyPressedEvent& e)
{
    if (e.GetKeyCode() == bee::key::Escape)
    {
        if (m_playing)
        {
#ifdef EDITOR_MODE
            StopApplication();
#else
            m_running = false;
#endif
        }
        else
        {
            m_running = false;
        }
    }

    if (e.GetKeyCode() == bee::key::F1)
    {
        m_device->HideCursor(!m_device->IsCursorHidden());
        bee::Log::Info("Cursor is now {}", m_device->IsCursorHidden() ? "hidden" : "visible");
    }

    return false;
}

bool bee::EngineClass::OnWindowResize(WindowResizeEvent& e)
{
#ifndef EDITOR_MODE
    m_registry.view<Camera>().each([&](auto, Camera& camera)
                                   { camera.SetAspectRatio((float)e.GetWidth() / (float)e.GetHeight()); });
#else
    (void)e;
#endif
    return false;
}



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/