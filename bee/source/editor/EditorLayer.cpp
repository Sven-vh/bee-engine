#include "editor/EditorLayer.hpp"
#include "managers/particle_manager.hpp"

#define CREATE_ENTITY_COMMAND(entity)                              \
    Scope<Command> cmd = CreateScope<CreateEntityCommand>(entity); \
    UndoRedoManager::Execute(std::move(cmd));

#define DELETE_ENTITY_COMMAND(entity)                              \
    Scope<Command> cmd = CreateScope<DeleteEntityCommand>(entity); \
    UndoRedoManager::Execute(std::move(cmd));

bee::EditorLayer::EditorLayer() : Layer("EditorLayer") {}

bee::EditorLayer::~EditorLayer()
{
    bee::Log::Info("EditorLayer destroyed");
    bee::Log::Info("EditorLayer destroyed");
}

void bee::EditorLayer::OnAttach()
{
    fs::path path = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, "Settings.json");
    std::ifstream is(path);
    if (is.good())
    {
        {
            cereal::JSONInputArchive archive(is);
            archive(m_editorSettings);
        }
    }
    is.close();

    UpdateCameraConfig(bee::Engine.EditorCamera());
}

void bee::EditorLayer::OnDetach()
{
    fs::path path = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, "Settings.json");
    std::ofstream os(path);
    {
        cereal::JSONOutputArchive archive(os);
        archive(m_editorSettings);
    }
    os.close();
}

void bee::EditorLayer::OnRender()
{
    DrawRaycastables();
    DrawIcons();
    DrawCameras();
    DrawGrids();
}

void bee::EditorLayer::OnUpdate(float deltatime)
{
    MoveEditorCamera(deltatime);

    UpdateGrids();
}

void bee::EditorLayer::OnFixedUpdate(float) {}

void bee::EditorLayer::OnImGuiRender()
{
    // dockspace
    static bool dockspaceOpen = true;
    static bool opt_fullscreen_persistant = true;
    bool opt_fullscreen = opt_fullscreen_persistant;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ApplyStyle(opt_fullscreen, dockspace_flags, dockspaceOpen);

    MainDockSpace(dockspace_flags);

    MenuBar();

    SceneHierarchy();

    Inspector();

    auto editorCamera = bee::Engine.EditorCamera();
    Viewport(ICON_FA_GLOBE TAB_FA "Editor", editorCamera, true);
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard && m_viewportEditorFocused)
    {
        io.WantCaptureKeyboard = false;
    }
    if (io.WantCaptureMouse && m_viewportEditorFocused)
    {
        io.WantCaptureMouse = false;
    }

    auto mainCamera = bee::Engine.MainCamera();
    if (mainCamera != entt::null && mainCamera != editorCamera)
    {
        // mainCamera = editorCamera;
        Viewport(ICON_FA_GAMEPAD TAB_FA "Main", mainCamera, false);
        if (io.WantCaptureKeyboard && m_viewportGameFocused)
        {
            io.WantCaptureKeyboard = false;
        }
        if (io.WantCaptureMouse && m_viewportGameFocused)
        {
            io.WantCaptureMouse = false;
        }
    }

    if (!bee::Engine.IsPlaying())
    {
        auto& registry = bee::Engine.Registry();
        // check if the selected entity is a camera
        auto view = registry.view<Camera>();
        for (auto entity : view)
        {
            bool isSelected = m_selectedEntities.find(entity) != m_selectedEntities.end();
            bool isPinned = m_pinnedCameras.find(entity) != m_pinnedCameras.end();

            if (isSelected || isPinned)
            {
                if (view.contains(entity))
                {
                    std::string cameraName =
                        (ICON_FA_VIDEO TAB_FA "Selected Camera##" + std::to_string(static_cast<int>(entity)));
                    Viewport(cameraName, entity, false);
                }
            }
        }
    }

    DrawEditorCamera();

    if (m_editorSettings.showProfiler) bee::profiler::OnImGuiRender();
    if (m_editorSettings.showResourceManager) bee::resource::OnImGuiRender();
    if (m_editorSettings.showLayoutWindow) LayoutEditor();
    if (m_editorSettings.showUndoRedoManager) UndoRedoManager::OnImGuiRender();
    if (m_editorSettings.showRenderStats) xsr::on_imgui_render();
    if (m_editorSettings.showRegistryStats) ShowRegistryStats();
    if (m_editorSettings.showAssetsBrowser) m_assetBrowser.OnImGuiRender();

    ImGui::End();
}

void bee::EditorLayer::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(EditorLayer::OnKeyPressed));
    dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
}

void bee::EditorLayer::ApplyStyle(bool opt_fullscreen, ImGuiDockNodeFlags dockspace_flags, bool& dockspaceOpen)
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |=
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen)
    {
        ImGui::PopStyleVar(2);
    }
}

void bee::EditorLayer::MainDockSpace(const ImGuiDockNodeFlags dockspace_flags)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }
}

void bee::EditorLayer::Viewport(const std::string& windowName, const entt::entity cameraEntity, bool editorViewport)
{
    // Set up viewport style and begin ImGui window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    // transparent background
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    // if window for first time set width and height when ImGuiCond_FirstUseEver
    ImGui::SetNextWindowSize(ImVec2(720, 480), ImGuiCond_FirstUseEver);

    // std::string title = "Viewport##" + std::to_string(static_cast<int>(cameraEntity));
    ImGui::Begin(windowName.c_str());
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    {
        // pop-up menu
        ImGui::OpenPopup("ViewportPopup");
        ImVec2 mousePos = ImGui::GetMousePos();
        ImGui::SetNextWindowPos(ImVec2(mousePos.x, mousePos.y));
    }
    if (ImGui::BeginPopup("ViewportPopup"))
    {
        bool isPinned = m_pinnedCameras.find(cameraEntity) != m_pinnedCameras.end();
        if (isPinned)
        {
            if (ImGui::MenuItem(ICON_FA_THUMBTACK TAB_FA "Unpin Camera"))
            {
                m_pinnedCameras.erase(cameraEntity);
            }
        }
        else
        {
            if (ImGui::MenuItem(ICON_FA_THUMBTACK TAB_FA "Pin Camera"))
            {
                m_pinnedCameras.insert(cameraEntity);
            }
        }
        ImGui::EndPopup();
    }

    /*if (editorViewport)*/

    ImGui::PushID(static_cast<int>(cameraEntity));
    // Determine if the viewport is focused

    if (editorViewport)
    {
        m_viewportEditorFocused = ImGui::IsWindowHovered();
        if (m_viewportEditorFocused) ImGui::SetWindowFocus();
    }
    else
    {
        m_viewportGameFocused = ImGui::IsWindowHovered();
        if (m_viewportGameFocused) ImGui::SetWindowFocus();
    }

    Ref<FrameBuffer> frameBuffer;

    if (m_frameBuffers.find(cameraEntity) != m_frameBuffers.end())
    {
        frameBuffer = m_frameBuffers[cameraEntity];
    }
    if (frameBuffer == nullptr)
    {
        FrameBufferSettings settings;
        settings.Width = 720;
        settings.Height = 480;
        frameBuffer = FrameBuffer::Create(settings);
        m_frameBuffers[cameraEntity] = frameBuffer;
        bee::Log::Warn("Creating new FrameBuffer for camera assets: {0}", (int)cameraEntity);
    }

    // Render the framebuffer image to the ImGui window
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

    if (viewportPanelSize.x < 0 || viewportPanelSize.y < 0)
    {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::PopID();
        ImGui::End();
        return;
    }

    ImGui::Image(reinterpret_cast<void*>(frameBuffer->GetColorAttachmentRendererID()),
                 viewportPanelSize,
                 ImVec2{0, 1},
                 ImVec2{1, 0});

    float viewportX = ImGui::GetWindowPos().x;
    float viewportY = ImGui::GetWindowPos().y;
    float titleBarHeight = ImGui::GetFrameHeightWithSpacing();

    if (editorViewport)
    {
        m_viewportPosition = {viewportX, viewportY};
        m_viewportSize = {viewportPanelSize.x, viewportPanelSize.y + titleBarHeight};
    }
    // Adjust framebuffer and camera if the viewport size has changed
    if (frameBuffer->GetSettings().Width != viewportPanelSize.x || frameBuffer->GetSettings().Height != viewportPanelSize.y)
    {
        frameBuffer->Bind();
        frameBuffer->Resize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
        frameBuffer->Unbind();
        if (bee::Engine.Registry().all_of<Camera>(cameraEntity))
        {
            auto& camera = bee::Engine.Registry().get<Camera>(cameraEntity);
            camera.SetAspectRatio(viewportPanelSize.x / viewportPanelSize.y);
        }
    }

    if (ImGui::IsWindowFocused())
    {
        ImGuizmo::SetID((int)cameraEntity);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(viewportX, viewportY, viewportPanelSize.x, viewportPanelSize.y);
        Gizmos(cameraEntity);
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::PopID();
    ImGui::End();

    // Update ImGui IO for input handling based on viewport focus

    if (!editorViewport)
    {
        return;
    }

    if (m_editorSettings.m_showToolbar)
    {
        // Set window size for the toolbar
        ImGui::SetNextWindowSize(ImVec2(viewportPanelSize.x * 0.95f, 0));
        std::string toolbarTitle = "Toolbar##" + std::to_string(static_cast<int>(cameraEntity));
        ImGui::Begin(toolbarTitle.c_str(),
                     nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::PushID(static_cast<int>(cameraEntity));

        // Position the window at the top left of the viewport
        float offsetX = (viewportPanelSize.x - ImGui::GetWindowSize().x) / 2;
        ImGui::SetWindowPos(ImVec2(viewportX + offsetX, viewportY + titleBarHeight));

        // Left side: Play/Pause, Step, and Time Scale
        bool paused = bee::Engine.IsPaused();

        if (ImGui::Button(paused ? ICON_FA_PLAY : ICON_FA_PAUSE))
        {
            paused ? bee::Engine.Resume() : bee::Engine.Pause();
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FORWARD_STEP))
        {
            float fixedDeltaTimeMS = 1.0f / bee::Engine.Settings().fixedUpdateRate;
            bee::Engine.FixedUpdate(fixedDeltaTimeMS);
            bee::Engine.Update(fixedDeltaTimeMS);
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(150);
        ImGui::SliderFloat("Time Scale", &bee::Engine.Settings().timeScale, 0.001f, 2.0f);

        // Centered "Start" and "Stop" buttons
        float toolbarWidth = ImGui::GetWindowWidth();
        float centerOffset = (toolbarWidth - (60 + 60 + 10)) / 2;  // Calculate center offset for buttons

        ImGui::SameLine(centerOffset);
        if (bee::Engine.IsPlaying())
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImFont* font = ImGui::GetIO().Fonts->Fonts[1];
            font->Scale = 1.5f;
            ImGui::PushFont(font);
            if (ImGui::Button(ICON_FA_STOP, ImVec2(30, 30)))
            {
                bee::Engine.StopApplication();

                if (!m_loadedSceneName.empty())
                {
                    auto& registry = bee::Engine.Registry();
                    auto path = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::SaveFiles, m_loadedSceneName);
                    bee::LoadRegistry(registry, path);
                }
            }
            ImGui::PopStyleColor();
            font->Scale = 1.0f;
            ImGui::PopFont();
        }
        else
        {
            // if (bee::ImGuiHelper::PlayButton("Start", ImVec2(50, 30)))
            // set button background invisible
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImFont* font = ImGui::GetIO().Fonts->Fonts[1];
            font->Scale = 1.5f;
            ImGui::PushFont(font);
            if (ImGui::Button(ICON_FA_PLAY, ImVec2(35, 30)))
            {
                auto& registry = bee::Engine.Registry();
                bool hasScene = !registry.storage<HierarchyNode>().empty();
                if (m_loadedSceneName.empty() && hasScene)
                {
                    m_sceneChangedPopup = true;
                }
                else
                {
                    if (hasScene)
                    {
                        m_openSaveAndPlayPopup = true;
                    }
                    else
                    {
                        bee::ecs::UnloadScene();
                        bee::Engine.StartApplication();
                    }
                }
            }
            ImGui::PopStyleColor();
            font->Scale = 1.0f;
            ImGui::PopFont();
        }

        // Right side: Additional Dropdowns
        const int dropDownCount = 3;

        ImGui::SameLine(ImGui::GetWindowWidth() - (160 * dropDownCount));  // Adjust offset to position buttons on the right

        ImGui::SetNextItemWidth(150);
        bee::Camera& camera = bee::Engine.Registry().get<Camera>(cameraEntity);
        bee::ImGuiHelper::EnumCombo("##CameraRenderMode", camera.renderMode);
        ImGui::SameLine();

        ImGui::SetNextItemWidth(150);

        const char* currentCameraType =
            m_editorSettings.m_currentCamType == CamType::Freefly ? ICON_FA_VIDEO TAB_FA "Freefly"
            : m_editorSettings.m_currentCamType == CamType::Orbit ? ICON_FA_ARROWS_TO_DOT TAB_FA "Orbit"
            : m_editorSettings.m_currentCamType == CamType::Up    ? ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT TAB_FA "Up"
            : m_editorSettings.m_currentCamType == CamType::Right ? ICON_FA_ARROW_RIGHT TAB_FA "Right"
                                                                  : ICON_FA_ARROW_UP TAB_FA "Forward";

        if (ImGui::BeginCombo("##CameraType", currentCameraType))
        {
            if (ImGui::Selectable(ICON_FA_VIDEO TAB_FA "Freefly", m_editorSettings.m_currentCamType == CamType::Freefly))
            {
                m_editorSettings.m_currentCamType = CamType::Freefly;
                UpdateCameraConfig(cameraEntity);
            }
            if (ImGui::Selectable(ICON_FA_ARROWS_TO_CIRCLE TAB_FA "Orbit", m_editorSettings.m_currentCamType == CamType::Orbit))
            {
                m_editorSettings.m_currentCamType = CamType::Orbit;
                UpdateCameraConfig(cameraEntity);
            }

            if (ImGui::Selectable(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT TAB_FA "Up",
                                  m_editorSettings.m_currentCamType == CamType::Up))
            {
                m_editorSettings.m_currentCamType = CamType::Up;
                UpdateCameraConfig(cameraEntity);
            }
            if (ImGui::Selectable(ICON_FA_ARROW_RIGHT TAB_FA "Right", m_editorSettings.m_currentCamType == CamType::Right))
            {
                m_editorSettings.m_currentCamType = CamType::Right;
                UpdateCameraConfig(cameraEntity);
            }
            if (ImGui::Selectable(ICON_FA_ARROW_UP TAB_FA "Forward", m_editorSettings.m_currentCamType == CamType::Forward))
            {
                m_editorSettings.m_currentCamType = CamType::Forward;
                UpdateCameraConfig(cameraEntity);
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(150);
        const char* currentOption =
            m_currentGizmoMode == ImGuizmo::MODE::LOCAL ? ICON_FA_CUBE TAB_FA "Local" : ICON_FA_GLOBE TAB_FA "World";
        if (ImGui::BeginCombo("##mode", currentOption))
        {
            if (ImGui::Selectable(ICON_FA_CUBE TAB_FA "Local", m_currentGizmoMode == ImGuizmo::MODE::LOCAL))
            {
                m_currentGizmoMode = ImGuizmo::MODE::LOCAL;
            }
            if (ImGui::Selectable(ICON_FA_GLOBE TAB_FA "World", m_currentGizmoMode == ImGuizmo::MODE::WORLD))
            {
                m_currentGizmoMode = ImGuizmo::MODE::WORLD;
            }
            ImGui::EndCombo();
        }

        ImGui::PopID();

        ImGui::End();
    }

    bool yes = false;
    bool no = false;

    if (m_sceneChangedPopup)
    {
        ImGui::OpenPopup("Scene Changed");
        if (ImGui::BeginPopupModal("Scene Changed", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Do you want to save the scene?");
            if (ImGui::Button("Yes"))
            {
                yes = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("No"))
            {
                no = true;
            }
            ImGui::EndPopup();
        }
    }

    if (yes)
    {
        if (!m_loadedSceneName.empty())
        {
            SaveScene();
        }
        bee::ecs::UnloadScene();
        m_sceneChangedPopup = false;
        bee::Engine.StartApplication();
    }
    else if (no)
    {
        bee::ecs::UnloadScene();
        m_sceneChangedPopup = false;
        bee::Engine.StartApplication();
    }

    if (m_openSaveAndPlayPopup)
    {
        ImGui::OpenPopup("Save and Play");
        if (ImGui::BeginPopupModal("Save and Play", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Do you want to save the scene before playing?");
            if (ImGui::Button("Yes"))
            {
                SaveScene();
                bee::ecs::UnloadScene();
                bee::Engine.StartApplication();
                m_openSaveAndPlayPopup = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("No"))
            {
                bee::ecs::UnloadScene();
                bee::Engine.StartApplication();
                m_openSaveAndPlayPopup = false;
            }
            ImGui::EndPopup();
        }
    }
}

void bee::EditorLayer::UpdateCameraConfig(const entt::entity cameraEntity) const
{
    switch (m_editorSettings.m_currentCamType)
    {
        case CamType::Freefly:
        {
            Camera& cam = bee::Engine.Registry().get<Camera>(cameraEntity);
            Transform& transform = bee::Engine.Registry().get<Transform>(cameraEntity);
            glm::vec3 newPos = transform.GetDirection() * -(cam.orthoSize * glm::pi<float>()) + transform.GetPosition();
            cam.orthographic = false;
            cam.nearClip = 0.1f;
            transform.SetPosition(newPos);
        }
        break;
        case CamType::Orbit:
        {
            Camera& camOrbit = bee::Engine.Registry().get<Camera>(cameraEntity);
            camOrbit.orthographic = true;
            camOrbit.nearClip = -100.0f;
            Transform& transformOrbit = bee::Engine.Registry().get<Transform>(cameraEntity);
            // set only y position to 0;
            glm::vec3 newPosOrbit = {transformOrbit.GetPosition().x, 0.0f, transformOrbit.GetPosition().z};
            transformOrbit.SetPosition(newPosOrbit);
        }
        break;
        case CamType::Up:
        {
            Camera& camUp = bee::Engine.Registry().get<Camera>(cameraEntity);
            camUp.orthographic = true;
            camUp.nearClip = -100.0f;
            Transform& transformUp = bee::Engine.Registry().get<Transform>(cameraEntity);
            transformUp.SetRotation({-90.0f, 0.0f, 0.0f});
            // set only x position to 0;
            glm::vec3 newPosUp = {0.0f, transformUp.GetPosition().y, transformUp.GetPosition().z};
            transformUp.SetPosition(newPosUp);
        }
        break;
        case CamType::Right:
        {
            Camera& camRight = bee::Engine.Registry().get<Camera>(cameraEntity);
            camRight.orthographic = true;
            camRight.nearClip = -100.0f;
            Transform& transformRight = bee::Engine.Registry().get<Transform>(cameraEntity);
            transformRight.SetRotation({0.0f, 90.0f, 0.0});
            // set only z position to 0;
            glm::vec3 newPosRight = {transformRight.GetPosition().x, transformRight.GetPosition().y, 0.0f};
            transformRight.SetPosition(newPosRight);
        }
        break;
        case CamType::Forward:
        {
            Camera& cam = bee::Engine.Registry().get<Camera>(cameraEntity);
            cam.orthographic = true;
            cam.nearClip = -100.0f;
            Transform& transform = bee::Engine.Registry().get<Transform>(cameraEntity);
            transform.SetRotation({0.0f, 0.0f, 0.0f});
        }
        break;
    }
}

// @Function: SceneHierarchy
void bee::EditorLayer::SceneHierarchy()
{
    static std::string entityName;  // Static variable to temporarily hold the entity name

    ImGui::Begin(ICON_FA_ALIGN_LEFT TAB_FA "Scene Hierarchy");

    // Detect click in empty space to clear selection, but only if Ctrl or Shift are not held
    if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) && !ImGui::GetIO().KeyCtrl &&
        !ImGui::GetIO().KeyShift && !m_renameWindow)
    {
        m_selectedEntities.clear();
        m_lastSelectedEntity = entt::null;  // Reset last selected entity when clearing
    }

    // Handle drag-and-drop to the window itself
    ImGuiID sceneHierarchyWindowID = ImGui::GetID("SceneHierarchyWindow");
    if (ImGui::GetCurrentWindow()->DockNode != nullptr &&
        ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->DockNode->Rect(), sceneHierarchyWindowID))
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_HIERARCHY_ENTITY"))
        {
            entt::entity droppedEntity = *(const entt::entity*)payload->Data;
            bee::ecs::RemoveParentChildRelationship(droppedEntity, bee::Engine.Registry());
        }
        ImGui::EndDragDropTarget();
    }

    // If Ctrl+D is pressed, duplicate selected entities
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_D)) && ImGui::GetIO().KeyCtrl && !m_selectedEntities.empty())
    {
        auto& registry = bee::Engine.Registry();
        bee::ecs::DuplicateSelectedEntities(registry, m_selectedEntities, m_selectedEntities);

        std::vector<Scope<Command>> commands;
        for (auto entity : m_selectedEntities)
        {
            commands.push_back(CreateScope<CreateEntityCommand>(entity));
        }
        UndoRedoManager::Execute(CreateScope<CommandCombination>(std::move(commands)));

        // deselect all the entities if one of their parents is selected
        for (auto it = m_selectedEntities.begin(); it != m_selectedEntities.end();)
        {
            std::vector<entt::entity> parents = bee::ecs::GetAllParents(registry, *it);
            bool parentSelected = false;

            for (auto parent : parents)
            {
                if (m_selectedEntities.find(parent) != m_selectedEntities.end())
                {
                    parentSelected = true;
                    break;
                }
            }

            if (parentSelected)
            {
                it = m_selectedEntities.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    // Check if delete key is pressed, if so delete selected entities
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)) && !m_selectedEntities.empty())
    {
        std::vector<Scope<Command>> commands;
        for (auto entity : m_selectedEntities)
        {
            commands.push_back(CreateScope<DeleteEntityCommand>(entity));
            // bee::ecs::DestroyEntity(entity, registry);
        }
        UndoRedoManager::Execute(CreateScope<CommandCombination>(std::move(commands)));
        m_selectedEntities.clear();
    }

    // Check if F2 key is pressed, if so prepare to rename selected entity
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F2)) && !m_selectedEntities.empty())
    {
        m_renameWindow = true;
    }

    if (m_renameWindow)
    {
        ImGui::OpenPopup("Rename Entity");
    }

    auto& registry = bee::Engine.Registry();
    entityName = PromptForName("Rename Entity", false);

    if (!entityName.empty())
    {
        for (auto entity : m_selectedEntities)
        {
            registry.get<HierarchyNode>(entity).name = entityName;
        }
        m_renameWindow = false;  // Reset the flag after renaming
    }

    auto view = registry.view<HierarchyNode>();

    for (auto entity : view)
    {
        auto& node = view.get<HierarchyNode>(entity);
        if (node.parent == entt::null)
        {
            DrawEntityNode(entity);
        }
    }

    ImGui::End();
}

void bee::EditorLayer::DrawEntityNode(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    auto& node = registry.get<HierarchyNode>(entity);

    // Unique ID for ImGui
    ImGui::PushID(static_cast<int>(entity));

    // Check if this entity is selected
    bool isSelected = m_selectedEntities.find(entity) != m_selectedEntities.end();

    // Determine flags for TreeNode
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (isSelected)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    // If this entity has no children, use the leaf flag
    if (node.children.empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    // Draw a tree node that is also selectable
    std::string label = ICON_FA_MINUS TAB_FA + node.name;
    bool nodeOpen = ImGui::TreeNodeEx(label.c_str(), flags);

    // Handle selection logic
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        if (ImGui::GetIO().KeyShift && m_lastSelectedEntity != entt::null)
        {
            // Shift is held, select all entities between the last selected and the current
            SelectEntitiesInRange(m_lastSelectedEntity, entity);
        }
        else if (ImGui::GetIO().KeyCtrl)
        {
            // Ctrl + Click for multi-selection
            if (isSelected)
            {
                m_selectedEntities.erase(entity);
            }
            else
            {
                m_selectedEntities.insert(entity);
            }
            m_lastSelectedEntity = entity;
        }
        else
        {
            // If neither Ctrl nor Shift is pressed, select only this entity
            m_selectedEntities.clear();
            m_selectedEntities.insert(entity);
            m_lastSelectedEntity = entity;
        }
    }

    // Drag-and-Drop Source Logic
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        ImGui::SetDragDropPayload("DND_HIERARCHY_ENTITY", &entity, sizeof(entt::entity));
        ImGui::Text("%s", node.name.c_str());
        ImGui::EndDragDropSource();
    }

    // Drag-and-Drop Target Logic
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_HIERARCHY_ENTITY"))
        {
            entt::entity droppedEntity = *(const entt::entity*)payload->Data;
            if (droppedEntity != entity)  // Prevent setting an entity as its own child
            {
                bee::ecs::SetParentChildRelationship(droppedEntity, entity, registry);  // Update parent-child relationship
            }
        }
        ImGui::EndDragDropTarget();
    }

    // If the node is open, recursively draw children
    if (nodeOpen)
    {
        for (auto& child : node.children)
        {
            DrawEntityNode(child);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void bee::EditorLayer::SelectEntitiesInRange(entt::entity first, entt::entity last)
{
    auto& registry = bee::Engine.Registry();
    auto view = registry.view<HierarchyNode>();

    bool selecting = false;
    for (auto entity : view)
    {
        if (entity == first || entity == last)
        {
            if (!selecting)
            {
                // Start selecting when the first entity is encountered
                selecting = true;
            }
            else
            {
                // Stop selecting when the range end is reached
                selecting = false;
                m_selectedEntities.insert(entity);
                break;
            }
        }

        if (selecting)
        {
            m_selectedEntities.insert(entity);
        }
    }

    // deselect all the entities if one of their parents is selected
    for (auto it = m_selectedEntities.begin(); it != m_selectedEntities.end();)
    {
        std::vector<entt::entity> parents = bee::ecs::GetAllParents(registry, *it);
        bool parentSelected = false;

        for (auto parent : parents)
        {
            if (m_selectedEntities.find(parent) != m_selectedEntities.end())
            {
                parentSelected = true;
                break;
            }
        }

        if (parentSelected)
        {
            it = m_selectedEntities.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void bee::EditorLayer::Inspector()
{
    ImGui::Begin(ICON_FA_CIRCLE_INFO TAB_FA "Inspector");

    auto& registry = bee::Engine.Registry();

    // Check if there are any selected entities
    if (m_selectedEntities.empty())
    {
        ImGui::Text("No assets selected.");
        ImGui::End();
        return;
    }

    entt::entity newSelectedEntity = entt::null;  // Temporary variable to hold new selection
    static entt::entity saveSelectedEntity = entt::null;
    // static std::string test = "test";
    //  Iterate over all selected entities
    bool multipleSelected = !m_selectedEntities.empty();
    for (auto entity : m_selectedEntities)
    {
        // ImGui::PushID(static_cast<int>(entity));  // Ensure unique ImGui ID for each entity

        bool isOpen = false;
        if (multipleSelected)
        {
            // only open the last one
            isOpen = entity == *m_selectedEntities.rbegin();
        }
        else
        {
            isOpen = ImGui::CollapsingHeader((("Entity " + std::to_string(static_cast<int>(entity))).c_str()),
                                             ImGuiTreeNodeFlags_DefaultOpen);
        }

        // Display entity header
        if (isOpen)
        {
            ComponentManager::DrawComponents(entity);
            ComponentManager::DrawAddComponents(entity);

            // save entity as prefab, button and popup
            if (ImGui::Button("Save as Prefab"))
            {
                m_savePrefabPopup = true;
                saveSelectedEntity = entity;
            }
        }
        else
        {
            ImVec2 oldPos = ImGui::GetCursorPos();                  // Save current cursor position
            ImGui::PushClipRect(ImVec2(0, 0), ImVec2(0, 0), true);  // Set an empty clip rect

            ComponentManager::DrawComponents(entity);
            ComponentManager::DrawAddComponents(entity);

            // save entity as prefab, button and popup
            if (ImGui::Button("Save as Prefab"))
            {
                m_savePrefabPopup = true;
                saveSelectedEntity = entity;
            }

            ImGui::PopClipRect();         // Restore the previous clipping region
            ImGui::SetCursorPos(oldPos);  // Restore cursor position
        }

        // ImGui::PopID();
    }

    // After the loop, if a new entity should be selected, clear the selection and set the new one
    if (newSelectedEntity != entt::null)
    {
        m_selectedEntities.clear();
        m_selectedEntities.insert(newSelectedEntity);
    }

    ImGui::End();

    if (m_savePrefabPopup)
    {
        ImGui::OpenPopup("Save as Prefab");
        if (ImGui::BeginPopupModal("Save as Prefab", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            std::string prefabName = PromptForName("Prefab Name", true);
            if (!prefabName.empty())
            {
                // get HierarchyNode
                auto& node = registry.get<HierarchyNode>(saveSelectedEntity);
                // remove from parent
                if (node.parent != entt::null)
                {
                    // check if parent is valid
                    if (!registry.valid(node.parent))
                    {
                        node.parent = entt::null;
                    }
                    else
                    {
                        auto& parentNode = registry.get<HierarchyNode>(node.parent);
                        parentNode.children.erase(
                            std::remove(parentNode.children.begin(), parentNode.children.end(), saveSelectedEntity),
                            parentNode.children.end());
                        node.parent = entt::null;
                    }
                }

                auto path = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::SaveFiles, prefabName + PREFAB_EXTENSION);
                bee::saveEntity(registry, saveSelectedEntity, path);
                m_savePrefabPopup = false;
            }
            ImGui::EndPopup();
        }
    }
}

void bee::EditorLayer::Gizmos(entt::entity cameraEntity)
{
    auto& registry = bee::Engine.Registry();
    auto view = registry.view<Transform, HierarchyNode>();

    if (m_selectedEntities.empty()) return;

    glm::vec3 center(0.0f);
    int count = 0;

    for (auto entity : m_selectedEntities)
    {
        if (view.contains(entity) && registry.valid(entity))
        {
            glm::mat4 model = GetWorldModel(entity, registry);
            center += glm::vec3(model[3]);
            ++count;
        }
    }

    if (count <= 0) return;

    if (count > 0)
    {
        center /= static_cast<float>(count);
    }

    glm::mat4 gizmoTransform = glm::translate(glm::mat4(1.0f), center);
    glm::mat4 deltaMatrix(1.0f);

    glm::vec3 initialRotationEuler(0.0f);
    if (view.contains(*m_selectedEntities.begin()))
    {
        glm::mat4 model = GetWorldModel(*m_selectedEntities.begin(), registry);

        glm::vec3 translation, scale;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), &translation[0], &initialRotationEuler[0], &scale[0]);

        gizmoTransform = glm::translate(glm::mat4(1.0f), translation) *
                         glm::mat4_cast(glm::quat(glm::radians(initialRotationEuler))) * glm::scale(glm::mat4(1.0f), scale);

        // also convert the deltaMatrict to the world space
        glm::vec3 deltaTranslation, deltaScale;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(deltaMatrix),
                                              &deltaTranslation[0],
                                              &initialRotationEuler[0],
                                              &deltaScale[0]);

        deltaMatrix = glm::translate(glm::mat4(1.0f), deltaTranslation) *
                      glm::mat4_cast(glm::quat(glm::radians(initialRotationEuler))) * glm::scale(glm::mat4(1.0f), deltaScale);
    }

    auto snapValues = glm::vec3(0.0f);
    bool useSnap = ImGui::GetIO().KeyCtrl;

    if (useSnap)
    {
        switch (m_currentGizmoOperation)
        {
            case ImGuizmo::OPERATION::TRANSLATE:
                snapValues = glm::vec3(m_editorSettings.m_positionRoundingValue);
                break;
            case ImGuizmo::OPERATION::ROTATE:
                snapValues = glm::vec3(m_editorSettings.m_rotationRoundingValue);
                break;
            case ImGuizmo::OPERATION::SCALEU:
                snapValues = glm::vec3(m_editorSettings.m_scaleRoundingValue);
                break;
            default:
                snapValues = glm::vec3(0.0f);
                break;
        }
    }

    // entt::entity cameraEntity = bee::Engine.EditorCamera();
    auto& camera = registry.get<Camera>(cameraEntity);
    auto& cameraTransform = registry.get<Transform>(cameraEntity);

    ImGuizmo::SetOrthographic(camera.orthographic);

    bool changed =
        ImGuizmo::Manipulate(value_ptr(camera.GetViewMatrix(cameraTransform.GetPosition(), cameraTransform.GetRotationQuat())),
                             value_ptr(camera.GetProjectionMatrix()),
                             m_currentGizmoOperation,
                             m_currentGizmoMode,
                             glm::value_ptr(gizmoTransform),
                             glm::value_ptr(deltaMatrix),
                             useSnap ? glm::value_ptr(snapValues) : nullptr);

    static bool isTransforming = false;
    static std::unordered_map<entt::entity, glm::mat4> initialTransforms;

    if (!isTransforming && ImGuizmo::IsUsing())
    {
        for (auto entity : m_selectedEntities)
        {
            if (view.contains(entity))
            {
                initialTransforms[entity] = GetWorldModel(entity, registry);
            }
        }

        isTransforming = true;
    }

    if (changed)
    {
        for (auto entity : m_selectedEntities)
        {
            if (view.contains(entity))
            {
                // Get the world model matrix
                glm::mat4 newWorldModel = deltaMatrix * GetWorldModel(entity, registry);
                SetWorldModel(entity, newWorldModel, registry);
            }
        }
    }

    if (isTransforming && !ImGuizmo::IsUsing())
    {
        std::vector<Scope<Command>> modelCommands;
        for (auto entity : m_selectedEntities)
        {
            if (view.contains(entity))
            {
                glm::mat4 newWorldModel = GetWorldModel(entity, registry);
                auto modelCmd = CreateScope<ModelMatrixCommand>(entity, initialTransforms[entity], newWorldModel);
                modelCommands.push_back(std::move(modelCmd));
            }
        }

        Scope<CommandCombination> combination = CreateScope<CommandCombination>(std::move(modelCommands));
        UndoRedoManager::Execute(std::move(combination));

        isTransforming = false;
        initialTransforms.clear();
    }
}

void bee::EditorLayer::MenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN TAB_FA "Open Scene..."))
            {
                // if (!sceneBrowsrInitialized)
                //{
                //     sceneBrowser.SetTypeFilters({SCENE_EXTENSION});
                //     sceneBrowser.SetPwd(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::SaveFiles, ""));
                //     sceneBrowsrInitialized = true;
                // }
                // sceneBrowser.Open();
                fs::path path = bee::FileDialog::OpenFile(FILE_FILTER("Scene Files", SCENE_EXTENSION));
                if (!path.empty())
                {
                    bee::ecs::UnloadScene();
                    bee::LoadRegistry(bee::Engine.Registry(), path);
                    m_loadedSceneName = path.filename().string();
                }
            }

            if (!m_loadedSceneName.empty() && ImGui::MenuItem(ICON_FA_FLOPPY_DISK TAB_FA "Save"))
            {
                SaveScene();
            }

            if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK TAB_FA "Save as..."))
            {
                fs::path path = bee::FileDialog::SaveFile(FILE_FILTER("Scene Files", SCENE_EXTENSION));
                if (!path.empty())
                {
                    if (path.extension() != SCENE_EXTENSION)
                    {
                        path += SCENE_EXTENSION;  // Append extension if missing
                    }
                    bee::saveRegistry(bee::Engine.Registry(), path);
                    m_loadedSceneName = path.filename().string();
                }
            }

            // unload scene
            if (ImGui::MenuItem(ICON_FA_FILE_MINUS TAB_FA "Unload Scene"))
            {
                bee::ecs::UnloadScene();
                m_loadedSceneName.clear();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Create"))
        {
            if (ImGui::MenuItem(ICON_FA_BOX TAB_FA "Load Prefab..."))
            {
                fs::path path = bee::FileDialog::OpenFile(FILE_FILTER("Prefab Files", PREFAB_EXTENSION));
                if (!path.empty())
                {
                    bee::loadEntity(bee::Engine.Registry(), path);
                }
            }

            if (ImGui::MenuItem(ICON_FA_CUBE TAB_FA "Load GLTF..."))
            {
                std::vector<fs::path> paths = bee::FileDialog::OpenFiles(FILE_MULTIPLE_FILTER("GLTF Files", ".gltf", ".glb"));
                for (auto& path : paths)
                {
                    auto gltfPath = bee::Engine.FileIO().GetPath(FileIO::Directory::None, path);
                    gltfPath = bee::Engine.FileIO().GetRelativePath(bee::FileIO::Directory::Root, gltfPath);
                    entt::entity newEntity = bee::resource::LoadGLTF(gltfPath, bee::Engine.Registry());
                    m_selectedEntities.insert(newEntity);
                    CREATE_ENTITY_COMMAND(newEntity);
                }
            }

            if (ImGui::MenuItem(ICON_FA_SHAPES TAB_FA "Empty Entity"))
            {
                auto entity = bee::ecs::CreateDefault();
                m_selectedEntities.clear();
                m_selectedEntities.insert(entity);
                CREATE_ENTITY_COMMAND(entity);
            }
            if (ImGui::MenuItem(ICON_FA_CUBE TAB_FA "3D Mesh"))
            {
                auto entity = bee::ecs::CreateMesh();
                m_selectedEntities.clear();
                m_selectedEntities.insert(entity);
                CREATE_ENTITY_COMMAND(entity);
            }
            if (ImGui::MenuItem(ICON_FA_WAND_MAGIC_SPARKLES TAB_FA "Emitter"))
            {
                auto entity = bee::ecs::CreateEmitter();
                m_selectedEntities.clear();
                m_selectedEntities.insert(entity);
                CREATE_ENTITY_COMMAND(entity);
            }
            if (ImGui::MenuItem(ICON_FA_SUN TAB_FA "Directional Light"))
            {
                auto entity = bee::ecs::CreateDirectionalLight();
                m_selectedEntities.clear();
                m_selectedEntities.insert(entity);
                CREATE_ENTITY_COMMAND(entity);
            }
            if (ImGui::MenuItem(ICON_FA_MOUNTAIN_SUN TAB_FA "Scene Data"))
            {
                auto entity = bee::ecs::CreateSceneData();
                m_selectedEntities.clear();
                m_selectedEntities.insert(entity);
                CREATE_ENTITY_COMMAND(entity);
            }
            if (ImGui::MenuItem(ICON_FA_VIDEO TAB_FA "Camera"))
            {
                auto entity = bee::ecs::CreateCamera();
                m_selectedEntities.clear();
                m_selectedEntities.insert(entity);
                CREATE_ENTITY_COMMAND(entity);
            }
            if (ImGui::MenuItem(ICON_FA_BORDER_ALL TAB_FA "Grid"))
            {
                auto entity = bee::ecs::CreateGrid();
                m_selectedEntities.clear();
                m_selectedEntities.insert(entity);
                CREATE_ENTITY_COMMAND(entity);
            }
            if (ImGui::MenuItem(ICON_FA_DIAGRAM_LEAN_CANVAS TAB_FA "Canvas"))
            {
                auto entity = bee::ecs::CreateCanvas();
                m_selectedEntities.clear();
                m_selectedEntities.insert(entity);
                CREATE_ENTITY_COMMAND(entity);
            }
            if (ImGui::MenuItem(ICON_FA_IMAGE TAB_FA "Image"))
            {
                auto entity = bee::ecs::CreateCanvasElement();
                m_selectedEntities.clear();
                m_selectedEntities.insert(entity);
                CREATE_ENTITY_COMMAND(entity);
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Engine"))
        {
            ImGui::Selectable(ICON_FA_TOOLBOX TAB_FA "Show Toolbar",
                              &m_editorSettings.m_showToolbar,
                              ImGuiSelectableFlags_DontClosePopups);
            ImGui::Selectable(ICON_FA_CLONE TAB_FA "Interpolate Physics",
                              &bee::Engine.Settings().interpolateParticles,
                              ImGuiSelectableFlags_DontClosePopups);
            // button that opens a window
            if (ImGui::MenuItem(ICON_FA_GEAR TAB_FA "Editor Settings"))
            {
                m_editorSettingsWindow = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Windows"))
        {
            ImGui::Selectable(ICON_FA_CHART_LINE TAB_FA "Profiler",
                              &m_editorSettings.showProfiler,
                              ImGuiSelectableFlags_DontClosePopups);
            ImGui::Selectable(ICON_FA_FILE_IMAGE TAB_FA "Resource Manager",
                              &m_editorSettings.showResourceManager,
                              ImGuiSelectableFlags_DontClosePopups);
            ImGui::Selectable(ICON_FA_TABLE_CELLS TAB_FA "Layout",
                              &m_editorSettings.showLayoutWindow,
                              ImGuiSelectableFlags_DontClosePopups);
            ImGui::Selectable(ICON_FA_ROTATE TAB_FA "Undo/Redo Manager",
                              &m_editorSettings.showUndoRedoManager,
                              ImGuiSelectableFlags_DontClosePopups);
            ImGui::Selectable(ICON_FA_IMAGE TAB_FA "Render Stats",
                              &m_editorSettings.showRenderStats,
                              ImGuiSelectableFlags_DontClosePopups);
            ImGui::Selectable(ICON_FA_CHART_BAR TAB_FA "Registry Stats",
                              &m_editorSettings.showRegistryStats,
                              ImGuiSelectableFlags_DontClosePopups);
            ImGui::Selectable(ICON_FA_CHESS_ROOK TAB_FA "Assets Browser",
                              &m_editorSettings.showAssetsBrowser,
                              ImGuiSelectableFlags_DontClosePopups);

            ImGui::EndMenu();
        }

        // fps counter all the way to the right, for now just use the ImGui fps counter. only update every 0.1 seconds
        static float fps;
        static double lastTime = 0.0f;
        double currentTime = ImGui::GetTime();
        if (currentTime - lastTime >= 0.1f)
        {
            lastTime = currentTime;
            fps = ImGui::GetIO().Framerate;
        }
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 75);
        ImGui::Text("%.1f FPS", fps);

        ImGui::EndMainMenuBar();
    }

    if (m_editorSettingsWindow) ImGui::OpenPopup("Editor Settings Window");
    // Settings window, but always set it in the center of the screen

    ImGui::SetNextWindowSize(ImVec2(400, 400));
    if (ImGui::BeginPopupModal("Editor Settings Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::Text("Fixed Update Rate");
        ImGui::SliderFloat("##fixedUpdateRate", &bee::Engine.Settings().fixedUpdateRate, 1.0f, 120.0f, "%.1f");
        ImGui::Text("Time Scale");
        ImGui::SliderFloat("##timeScale", &bee::Engine.Settings().timeScale, 0.001f, 2.0f, "%.3f");
        ImGui::Text("Interpolate Physics");
        ImGui::Checkbox("##interpolatePhysics", &bee::Engine.Settings().interpolateParticles);
        ImGui::BeginTable("##SettingsTable", 4);
        ImGui::TableNextColumn();
        ImGui::Text("Show Hitboxes");
        ImGui::TableNextColumn();
        ImGui::Checkbox("##showHitboxes", &m_editorSettings.m_showHitboxes);
        ImGui::TableNextColumn();
        ImGui::Text("Show Icons");
        ImGui::TableNextColumn();
        ImGui::Checkbox("##showIcons", &m_editorSettings.m_showIcons);
        ImGui::EndTable();

        ImGui::BeginTable("##SettingsTable2", 4);
        ImGui::TableNextColumn();
        ImGui::Text("Show Cameras");
        ImGui::TableNextColumn();
        ImGui::Checkbox("##showHitboxes", &m_editorSettings.m_showCameras);
        ImGui::TableNextColumn();
        // ImGui::Text("Show Icons");
        ImGui::TableNextColumn();
        // ImGui::Checkbox("##showIcons", &m_showIcons);
        ImGui::EndTable();

        if (ImGui::CollapsingHeader(ICON_FA_SLIDERS TAB_FA "Transform Rounding"))
        {
            ImGui::Text("Position Rounding");
            ImGui::SliderFloat("##positionRounding", &m_editorSettings.m_positionRoundingValue, 0.0f, 10.0f, "%.1f");
            ImGui::Text("Rotation Rounding");
            ImGui::SliderFloat("##rotationRounding", &m_editorSettings.m_rotationRoundingValue, 0.0f, 90.0f, "%.1f");
            ImGui::Text("Scale Rounding");
            ImGui::SliderFloat("##scaleRounding", &m_editorSettings.m_scaleRoundingValue, 0.0f, 5.0f, "%.1f");
            ImGui::Separator();
        }

#ifdef EDITOR_MODE
        if (ImGui::CollapsingHeader(ICON_FA_BORDER_ALL TAB_FA "Grid"))
        {
            xsr::get_grid_settings().OnImGuiRender();
            ImGui::Separator();
        }
#endif

        if (ImGui::CollapsingHeader(ICON_FA_VIDEO TAB_FA "Camera"))
        {
            // m_cameraSettingsWindow
            ImGui::Checkbox("Show Camera Specs", &m_cameraSettingsWindow);
        }
        // if (ImGui::CollapsingHeader("Skybox"))
        //{
        //     if (m_skyColor.OnImGuiRender())
        //     {
        //         xsr::set_background_gradient(m_skyColor);
        //     }
        // }
        if (ImGui::Button("Close") || m_editorSettingsWindow == false)
        {
            m_editorSettingsWindow = false;
            ImGui::CloseCurrentPopup();
        }

        ImVec2 windowSize = ImGui::GetWindowSize();
        ImVec2 windowPos =
            ImVec2(ImGui::GetIO().DisplaySize.x / 2 - windowSize.x / 2, ImGui::GetIO().DisplaySize.y / 2 - windowSize.y / 2);
        ImGui::SetWindowPos(windowPos);

        ImGui::EndPopup();
    }
}

void bee::EditorLayer::SaveSceneAs(std::string& saveName)
{
    auto& registry = bee::Engine.Registry();
    auto path = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::SaveFiles, saveName + SCENE_EXTENSION);
    bee::saveRegistry(registry, path);
    m_loadedSceneName = bee::Engine.FileIO().GetRelativePath(bee::FileIO::Directory::SaveFiles, path);
}

void bee::EditorLayer::DrawEditorCamera()
{
    if (!m_cameraSettingsWindow) return;
    ImGui::Begin(ICON_FA_CAMERA TAB_FA "Editor Camera");

    entt::entity cameraEntity = bee::Engine.EditorCamera();
    ComponentManager::DrawComponents(cameraEntity);

    // sensitivty
    ImGui::SliderFloat("Sensitivity", &m_editorSettings.m_cameraSensitivity, 0.1f, 1.0f);

    ImGui::End();
}

void bee::EditorLayer::MoveEditorCamera(float deltatime)
{
    if (m_viewportGameFocused)
    {
        return;
    }

    deltatime /= bee::Engine.Settings().timeScale;

    entt::entity cameraEntity = bee::Engine.EditorCamera();
    auto& cameraTransform = bee::Engine.Registry().get<Transform>(cameraEntity);
    bee::Input& input = bee::Engine.Input();

    // if right mouse button is pressed, enable movement control
    if (!input.GetMouseButton(bee::Input::MouseButton::Right))
    {
        // if (bee::Engine.Device().IsCursorHidden())
        //{
        //     bee::Engine.Device().HideCursor(false);
        // }
        return;
    }

    // if (!bee::Engine.Device().IsCursorHidden())
    //{
    //     bee::Engine.Device().HideCursor(true);
    // }
    //  else
    //{
    //      bee::Engine.Device().HideCursor(false);
    //  }

    switch (m_editorSettings.m_currentCamType)
    {
        case CamType::Freefly:
        {
            // Fly around freely
            if (input.GetKeyboardKey(bee::Input::KeyboardKey::W))
            {
                cameraTransform.SetPosition(cameraTransform.GetPosition() +
                                            cameraTransform.GetDirection() * m_editorSettings.m_cameraSpeed * deltatime);
            }
            if (input.GetKeyboardKey(bee::Input::KeyboardKey::S))
            {
                cameraTransform.SetPosition(cameraTransform.GetPosition() -
                                            cameraTransform.GetDirection() * m_editorSettings.m_cameraSpeed * deltatime);
            }
            if (input.GetKeyboardKey(bee::Input::KeyboardKey::A))
            {
                cameraTransform.SetPosition(cameraTransform.GetPosition() -
                                            cameraTransform.GetRight() * m_editorSettings.m_cameraSpeed * deltatime);
            }
            if (input.GetKeyboardKey(bee::Input::KeyboardKey::D))
            {
                cameraTransform.SetPosition(cameraTransform.GetPosition() +
                                            cameraTransform.GetRight() * m_editorSettings.m_cameraSpeed * deltatime);
            }
            if (input.GetKeyboardKey(bee::Input::KeyboardKey::Q))
            {
                cameraTransform.SetPosition(cameraTransform.GetPosition() -
                                            cameraTransform.GetUp() * m_editorSettings.m_cameraSpeed * deltatime);
            }
            if (input.GetKeyboardKey(bee::Input::KeyboardKey::E))
            {
                cameraTransform.SetPosition(cameraTransform.GetPosition() +
                                            cameraTransform.GetUp() * m_editorSettings.m_cameraSpeed * deltatime);
            }
            m_editorSettings.m_cameraSpeed *= pow(1.5f, input.GetMouseWheel());

            // Rotate camera freely with mouse movement
            glm::vec2 before = input.GetPreviousMousePosition();
            glm::vec2 after = input.GetMousePosition();
            glm::vec2 mouse_delta = after - before;
            cameraTransform.SetRotation(cameraTransform.GetRotationEuler() -
                                        glm::vec3(mouse_delta.y, mouse_delta.x, 0.0f) * m_editorSettings.m_cameraSensitivity);
            break;
        }
        case CamType::Orbit:
        {
            // Move only on the xz-plane (ignore y-axis movement)
            Camera& cam = bee::Engine.Registry().get<Camera>(cameraEntity);
            if (input.GetKeyboardKey(bee::Input::KeyboardKey::W))
            {
                cameraTransform.SetPosition(cameraTransform.GetPosition() + glm::vec3(cameraTransform.GetDirection().x,
                                                                                      0.0f,
                                                                                      cameraTransform.GetDirection().z) *
                                                                                cam.orthoSize * deltatime);
            }
            if (input.GetKeyboardKey(bee::Input::KeyboardKey::S))
            {
                cameraTransform.SetPosition(cameraTransform.GetPosition() - glm::vec3(cameraTransform.GetDirection().x,
                                                                                      0.0f,
                                                                                      cameraTransform.GetDirection().z) *
                                                                                cam.orthoSize * deltatime);
            }
            if (input.GetKeyboardKey(bee::Input::KeyboardKey::A))
            {
                cameraTransform.SetPosition(cameraTransform.GetPosition() -
                                            cameraTransform.GetRight() * cam.orthoSize * deltatime);
            }
            if (input.GetKeyboardKey(bee::Input::KeyboardKey::D))
            {
                cameraTransform.SetPosition(cameraTransform.GetPosition() +
                                            cameraTransform.GetRight() * cam.orthoSize * deltatime);
            }
            cam.orthoSize *= pow(1.5f, -input.GetMouseWheel());
            // Rotate camera freely with mouse movement
            glm::vec2 before = input.GetPreviousMousePosition();
            glm::vec2 after = input.GetMousePosition();
            glm::vec2 mouse_delta = after - before;
            cameraTransform.SetRotation(cameraTransform.GetRotationEuler() -
                                        glm::vec3(mouse_delta.y, mouse_delta.x, 0.0f) * m_editorSettings.m_cameraSensitivity);
            break;
        }
        case CamType::Up:
        case CamType::Right:
        case CamType::Forward:
        {
            // For these views, disable rotation and allow only lateral movement
            if (input.GetKeyboardKey(bee::Input::KeyboardKey::A))
            {
                cameraTransform.SetPosition(cameraTransform.GetPosition() -
                                            cameraTransform.GetRight() * m_editorSettings.m_cameraSpeed * deltatime);
            }
            if (input.GetKeyboardKey(bee::Input::KeyboardKey::D))
            {
                cameraTransform.SetPosition(cameraTransform.GetPosition() +
                                            cameraTransform.GetRight() * m_editorSettings.m_cameraSpeed * deltatime);
            }
            if (input.GetKeyboardKey(bee::Input::KeyboardKey::W))
            {
                cameraTransform.SetPosition(cameraTransform.GetPosition() +
                                            cameraTransform.GetUp() * m_editorSettings.m_cameraSpeed * deltatime);
            }
            if (input.GetKeyboardKey(bee::Input::KeyboardKey::S))
            {
                cameraTransform.SetPosition(cameraTransform.GetPosition() -
                                            cameraTransform.GetUp() * m_editorSettings.m_cameraSpeed * deltatime);
            }
            Camera& cam = bee::Engine.Registry().get<Camera>(cameraEntity);
            cam.orthoSize *= pow(1.5f, -input.GetMouseWheel());

            glm::vec2 before = input.GetPreviousMousePosition();
            glm::vec2 after = input.GetMousePosition();
            glm::vec2 mouse_delta = after - before;

            const float movementScaleX = 2.0f * cam.orthoSize / m_viewportSize.x * cam.aspectRatio;
            const float movementScaleY = 2.0f * cam.orthoSize / m_viewportSize.y;

            switch (m_editorSettings.m_currentCamType)
            {
                case CamType::Up:
                    cameraTransform.SetPosition(cameraTransform.GetPosition() + glm::vec3(-mouse_delta.x * movementScaleX,
                                                                                          0.0f,
                                                                                          -mouse_delta.y * movementScaleY));
                    break;
                case CamType::Right:
                    cameraTransform.SetPosition(cameraTransform.GetPosition() + glm::vec3(0.0f,
                                                                                          mouse_delta.y * movementScaleY,
                                                                                          mouse_delta.x * movementScaleX));
                    break;
                case CamType::Forward:
                    cameraTransform.SetPosition(cameraTransform.GetPosition() + glm::vec3(-mouse_delta.x * movementScaleX,
                                                                                          mouse_delta.y * movementScaleY,
                                                                                          0.0f));
                    break;
                default:
                    break;
            }

            break;
        }
        default:
            break;
    }
}

void bee::EditorLayer::SaveScene()
{
    auto& registry = bee::Engine.Registry();
    auto path = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::SaveFiles, m_loadedSceneName.filename());
    bee::saveRegistry(registry, path);
}

void bee::EditorLayer::Begin(const entt::entity cameraEntity)
{
    // m_frameBuffer->Bind();

    if (m_frameBuffers.find(cameraEntity) != m_frameBuffers.end())
    {
        Ref<FrameBuffer> framebuffer = m_frameBuffers[cameraEntity];
        framebuffer->Bind();
    }
    else
    {
        // Handle case where the framebuffer is not found
        bee::Log::Warn("Framebuffer not found for camera: {0}, Creating one.", (int)cameraEntity);
        FrameBufferSettings spec;
        spec.Width = 1280;
        spec.Height = 720;
        m_frameBuffers[cameraEntity] = FrameBuffer::Create(spec);
        m_frameBuffers[cameraEntity]->Bind();
    }

    ImGuizmo::SetOrthographic(bee::Engine.Registry().get<Camera>(cameraEntity).orthographic);
}

void bee::EditorLayer::End(const entt::entity cameraEntity)
{
    if (m_frameBuffers.find(cameraEntity) != m_frameBuffers.end())
    {
        m_frameBuffers[cameraEntity]->Unbind();
    }
    else
    {
        bee::Log::Warn("Framebuffer not found for camera assets: {0}", (int)cameraEntity);
    }
}

bool bee::EditorLayer::OnKeyPressed(KeyPressedEvent& e)
{
    if (m_editorSettingsWindow || m_renameWindow)
    {
        if (e.GetKeyCode() == bee::key::Escape)
        {
            m_editorSettingsWindow = false;
            m_renameWindow = false;
        }
        e.handled = true;
        return true;
    }

    bool ctrlDown = bee::Engine.Input().GetKeyboardKey(bee::Input::KeyboardKey::LeftControl);
    bool shiftDown = bee::Engine.Input().GetKeyboardKey(bee::Input::KeyboardKey::LeftShift);

    switch (e.GetKeyCode())
    {
        case bee::key::Z:
            if (ctrlDown && !shiftDown)
            {
                UndoRedoManager::Undo();
                bee::ecs::MarkAllAsDirty(bee::Engine.Registry());
            }
            if (ctrlDown && shiftDown)
            {
                UndoRedoManager::Redo();
                bee::ecs::MarkAllAsDirty(bee::Engine.Registry());
            }
            break;
        case bee::key::Y:
            if (ctrlDown)
            {
                UndoRedoManager::Redo();
                bee::ecs::MarkAllAsDirty(bee::Engine.Registry());
            }
            break;
        default:
            break;
    }

    bool rightClickDown = ImGui::IsMouseDown(1);
    if (!m_viewportEditorFocused || rightClickDown) return false;

    switch (e.GetKeyCode())
    {
        case bee::key::W:
            m_currentGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
            break;
        case bee::key::E:
            m_currentGizmoOperation = ImGuizmo::OPERATION::ROTATE;
            break;
        case bee::key::R:
            m_currentGizmoOperation = ImGuizmo::OPERATION::SCALEU;
            break;
        case bee::key::T:
            m_currentGizmoOperation = ImGuizmo::OPERATION::UNIVERSAL;
            break;
        case bee::key::F:
        {
            entt::entity cameraEntity = bee::Engine.EditorCamera();
            bee::Transform cameraTransform = bee::Engine.Registry().get<Transform>(cameraEntity);
            cameraTransform.SetPosition(GetAveragePositionOfSelectedEntities());
            break;
        }
        default:
            break;
    }
    return false;
}

glm::vec3 bee::EditorLayer::GetAveragePositionOfSelectedEntities()
{
    if (m_selectedEntities.empty()) return glm::vec3(0.0f);  // Return zero vector if no entities are selected
    glm::vec3 totalPosition(0.0f);
    for (auto entity : m_selectedEntities)
    {
        auto& transform = bee::Engine.Registry().get<Transform>(entity);
        totalPosition += transform.GetPosition();
    }
    return totalPosition / static_cast<float>(m_selectedEntities.size());
}

bool bee::EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
{
    // IsOver somehow return true someitmes https://github.com/CedricGuillemet/ImGuizmo/issues/133
    bool usingGizmo = ImGuizmo::IsUsing() || ImGuizmo::IsOver();
    if (e.GetMouseButton() == bee::mouse::ButtonLeft && m_viewportEditorFocused && !m_editorSettingsWindow && !m_renameWindow &&
        !usingGizmo)
    {
        glm::vec2 screenPoint = glm::vec2(e.GetX(), e.GetY()) - m_viewportPosition;

        entt::entity cameraEntity = bee::Engine.EditorCamera();
        auto& camera = bee::Engine.Registry().get<Camera>(cameraEntity);
        auto& cameraTransform = bee::Engine.Registry().get<Transform>(cameraEntity);
        m_previousRay = camera.ScreenPointToRay(screenPoint,
                                                cameraTransform.GetPosition(),
                                                cameraTransform.GetRotationQuat(),
                                                m_viewportSize.x,
                                                m_viewportSize.y);
        bool hasHit = CheckGridIntersections();
        if (!hasHit)
        {
            CheckEntityIntersections();
        }
    }
    return false;
}

bool bee::EditorLayer::CheckGridIntersections()
{
    auto& registry = bee::Engine.Registry();
    auto view = registry.view<Transform, Raycastable, Cell>();

    for (auto entity : view)
    {
        auto& raycastable = view.get<Raycastable>(entity);
        if (!raycastable.raycastable) continue;

        Cell& cell = view.get<Cell>(entity);
        Grid& grid = registry.get<Grid>(cell.gridParent);

        Transform& cellTransform = view.get<Transform>(entity);

        glm::mat4 modelMatrix = GetWorldModel(cell.gridParent, registry) * cellTransform.GetModelMatrix();

        raycasting::HitInfo hitInfo = raycasting::IntersectRayWithEntity(m_previousRay, modelMatrix, glm::vec3(grid.tileSize));

        if (hitInfo.hit)
        {
            bee::Log::Info("Hit Grid");

            if (cell.entity != entt::null)
            {
                bee::ecs::DestroyEntity(cell.entity, registry);
            }

            // load gltf file from selected asset item
            entt::entity selectedAssetEntity = m_assetBrowser.GetSelected();
            if (selectedAssetEntity != entt::null)
            {
                AssetItem& assetItem = registry.get<AssetItem>(selectedAssetEntity);
                auto path = assetItem.path;
                entt::entity newEntity = bee::resource::LoadGLTF(path, bee::Engine.Registry());

                cell.entity = newEntity;

                Transform& newEntityTransform = registry.get<Transform>(newEntity);
                newEntityTransform.SetPosition(cellTransform.GetPosition());

                bee::ecs::SetParentChildRelationship(newEntity, cell.gridParent, registry, true);
            }

            return true;
        }
    }

    return false;
}

void bee::EditorLayer::UpdateGrids()
{
    bool usingGizmo = ImGuizmo::IsUsing() || ImGuizmo::IsOver();
    bool isMouseButtonDown = ImGui::IsMouseDown(0);
    if (isMouseButtonDown && m_viewportEditorFocused && !m_editorSettingsWindow && !m_renameWindow && !usingGizmo)
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        glm::vec2 screenPoint = glm::vec2(mousePos.x, mousePos.y) - m_viewportPosition;

        entt::entity cameraEntity = bee::Engine.EditorCamera();
        auto& camera = bee::Engine.Registry().get<Camera>(cameraEntity);
        auto& cameraTransform = bee::Engine.Registry().get<Transform>(cameraEntity);
        m_previousRay = camera.ScreenPointToRay(screenPoint,
                                                cameraTransform.GetPosition(),
                                                cameraTransform.GetRotationQuat(),
                                                m_viewportSize.x,
                                                m_viewportSize.y);
        CheckGridIntersections();
    }
}

void bee::EditorLayer::CheckEntityIntersections()
{
    entt::entity closestEntity = entt::null;
    float closestDistance = std::numeric_limits<float>::max();

    auto& registry = bee::Engine.Registry();
    auto view = registry.view<Transform, Raycastable>(entt::exclude<Cell>);
    for (auto entity : view)
    {
        glm::vec3 scaler = raycasting::GetEntityScaler(registry, entity);
        glm::vec3 offset = raycasting::GetEntityOffset(registry, entity);

        // check if it has a parent
        glm::mat4 modelMatrix = GetWorldModel(entity, registry);
        modelMatrix = glm::translate(modelMatrix, offset);

        raycasting::HitInfo hitInfo = raycasting::IntersectRayWithEntity(m_previousRay, modelMatrix, scaler);

        if (hitInfo.hit && hitInfo.distance < closestDistance)
        {
            closestDistance = hitInfo.distance;
            closestEntity = entity;
        }
    }

    if (closestEntity != entt::null)
    {
        if (ImGui::GetIO().KeyCtrl)
        {
            if (m_selectedEntities.find(closestEntity) != m_selectedEntities.end())
            {
                entt::entity toBeDeselected = bee::ecs::GetUppestSelectableParent(registry, closestEntity);
                if (toBeDeselected != entt::null)
                {
                    m_selectedEntities.erase(toBeDeselected);
                }
            }
            else
            {
                entt::entity toBeSelected = bee::ecs::GetUppestSelectableParent(registry, closestEntity);
                if (toBeSelected != entt::null)
                {
                    m_selectedEntities.insert(toBeSelected);
                }
            }
        }
        else
        {
            m_selectedEntities.clear();
            entt::entity toBeSelected = bee::ecs::GetUppestSelectableParent(registry, closestEntity);
            if (toBeSelected != entt::null)
            {
                m_selectedEntities.insert(toBeSelected);
            }
        }
    }
    else
    {
        m_selectedEntities.clear();
    }
}

void bee::EditorLayer::DrawRaycastables() const
{
    if (!m_editorSettings.m_showHitboxes) return;

    auto& registry = bee::Engine.Registry();
    auto view = registry.view<Transform, Raycastable>(entt::exclude<Cell>);

    for (auto entity : view)
    {
        auto& raycastable = view.get<Raycastable>(entity);
        glm::vec3 scaler = raycasting::GetEntityScaler(registry, entity);
        glm::vec3 offset = raycasting::GetEntityOffset(registry, entity);

        if (raycastable.raycastable)
        {
            glm::mat4 modelMatrix = GetWorldModel(entity, registry);
            modelMatrix = glm::translate(modelMatrix, offset);
            DebugRenderer::DrawTransformedBox(modelMatrix, scaler, glm::vec4(1.0f));
        }
    }

    xsr::render_debug_line(glm::value_ptr(m_previousRay.origin),
                           glm::value_ptr(m_previousRay.origin + m_previousRay.direction * 100.0f),
                           glm::value_ptr(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)));
}

void bee::EditorLayer::DrawIcons() const
{
    if (!m_editorSettings.m_showIcons) return;
    auto& registry = bee::Engine.Registry();
    auto view = registry.view<Transform, EditorIcon>();

    entt::entity cameraEntity = bee::Engine.EditorCamera();
    auto& cameraTransform = bee::Engine.Registry().get<Transform>(cameraEntity);
    glm::vec3 cameraPosition = cameraTransform.GetPosition();
    // xsr::mesh_handle quad = xsr::create_default_quad_mesh();
    for (auto entity : view)
    {
        glm::mat4 model = GetWorldModel(entity, registry);
        auto& icon = view.get<EditorIcon>(entity);

        model = bee::helper::LookToPosition(model, cameraPosition, false, glm::vec3(1.0f));

        xsr::render_mesh(glm::value_ptr(model),
                         icon.quad->GetHandle(),
                         icon.icon->GetHandle(),
                         glm::value_ptr(glm::float4(1.0f, 1.0f, 1.0f, 0.95f)),
                         glm::value_ptr(glm::vec4(0.1f)),
                         false);
    }
}

void bee::EditorLayer::DrawCameras() const
{
    if (!m_editorSettings.m_showCameras) return;
    auto& registry = bee::Engine.Registry();
    auto view = registry.view<Transform, Camera>();

    auto editorCameraEntity = bee::Engine.EditorCamera();
    for (auto entity : view)
    {
        if (entity == editorCameraEntity) continue;
        auto& camera = view.get<Camera>(entity);

        glm::mat4 cameraModel = GetWorldModel(entity, registry);
        const glm::vec3 cameraPosition = cameraModel[3];
        glm::quat rotation = glm::quat_cast(cameraModel);

        glm::mat4 viewMatrix = camera.GetViewMatrix(cameraPosition, rotation);
        glm::mat4 projectionMatrix = camera.GetProjectionMatrix();

        bee::DebugRenderer::DrawFrustum(viewMatrix, projectionMatrix, glm::vec4(1.0f), camera.orthographic);
    }
}

void bee::EditorLayer::DrawGrids() { GridManager::DrawCells(bee::Engine.Registry()); }

void bee::EditorLayer::ShowRegistryStats()
{
    auto& registry = bee::Engine.Registry();

    // Store the selected component as a static or class-level variable
    static const char* selected_component_name = nullptr;
    static entt::id_type selected_component_id = 0;

    // Begin ImGui window for registry statistics
    if (ImGui::Begin(ICON_FA_CHART_BAR TAB_FA "Registry Stats"))
    {
        // Display total number of entities using the correct function
        ImGui::Text("Total Entities: %d", static_cast<int>(registry.storage<entt::entity>().in_use()));

        // Iterate through each storage (i.e., each component type) in the registry
        ImGui::Separator();
        ImGui::Text("Component Breakdown:");
        ImGui::Separator();

        // Go through all component storages to gather statistics
        for (auto&& [component_id, storage] : registry.storage())
        {
            // Get the meta type for the component (this assumes you've registered components with entt::meta)
            auto meta_type = entt::resolve(component_id);

            if (!meta_type)
            {
                continue;
            }

            // Get the name of the component and the number of entities that have it
            std::string component_name = bee::ComponentManager::CleanTypeName(meta_type.info().name().data());
            size_t component_count = storage.size();

            // Display component name and count
            ImGui::Text("%s: %zu entities", component_name.c_str(), component_count);
        }

        ImGui::Separator();

        // You could add additional stats here, like average components per entity, memory usage, etc.
        ImGui::Separator();
        ImGui::Text("Entities for a selected component:");
        if (ImGui::BeginCombo("Select Component", selected_component_name ? selected_component_name : "Select Component"))
        {
            for (auto&& [component_id, storage] : registry.storage())
            {
                // Get the meta type for the component
                auto meta_type = entt::resolve(component_id);

                if (!meta_type)
                {
                    continue;
                }

                const char* component_name = meta_type.info().name().data();

                // If the user selects a component, store its name and ID
                if (ImGui::Selectable(component_name, selected_component_name == component_name))
                {
                    selected_component_name = component_name;
                    selected_component_id = component_id;
                }
            }
            ImGui::EndCombo();
        }

        if (selected_component_name && selected_component_id)
        {
            ImGui::Text("Entities with %s:", selected_component_name);

            // Display entities that have the selected component
            auto* selected_storage = registry.storage(selected_component_id);
            if (selected_storage)
            {
                // for (auto entity : *selected_storage)
                //{
                //     ImGui::Text("Entity ID: %d", static_cast<int>(entity));
                // }
                // inversed for loop
                for (auto it = selected_storage->rbegin(); it != selected_storage->rend(); ++it)
                {
                    ImGui::Text("Entity ID: %d", static_cast<int>(*it));

                    if (registry.all_of<HierarchyNode>(*it))
                    {
                        ImGui::SameLine();
                        ImGui::PushID(static_cast<int>(*it));
                        if (ImGui::Button("Select"))
                        {
                            m_selectedEntities.clear();
                            m_selectedEntities.insert(*it);
                        }
                        ImGui::PopID();
                    }
                }
            }
        }
    }
    // End ImGui window
    ImGui::End();
}

void bee::EditorLayer::LayoutEditor()
{
    ImGui::Begin(ICON_FA_BORDER_ALL TAB_FA "Layout Editor");

    ImGui::Checkbox("Auto Size", &m_editorSettings.layoutAutoSize);

    if (!m_editorSettings.layoutAutoSize)
    {
        // imgui dropdown for width or height
        if (ImGui::BeginCombo("##Size", m_editorSettings.layoutUseWidth ? "Width" : "Height"))
        {
            if (ImGui::Selectable("Width", m_editorSettings.layoutUseWidth))
            {
                m_editorSettings.layoutUseWidth = true;
            }
            if (ImGui::Selectable("Height", !m_editorSettings.layoutUseWidth))
            {
                m_editorSettings.layoutUseWidth = false;
            }
            ImGui::EndCombo();
        }

        if (m_editorSettings.layoutUseWidth)
        {
            ImGui::InputInt("Cols", &m_editorSettings.layoutCols);
        }
        else
        {
            ImGui::InputInt("Rows", &m_editorSettings.layoutRows);
        }
    }

    ImGui::SliderFloat("Spacing", &m_editorSettings.layoutSpacing, 0.0f, 5.0f);

    // apply button
    if (ImGui::Button("Apply"))
    {
        int selectedCount = (int)m_selectedEntities.size();

        if (m_editorSettings.layoutAutoSize)
        {
            auto sideLength = (float)sqrt(selectedCount);
            m_editorSettings.layoutRows = static_cast<int>(ceil(sideLength));
            m_editorSettings.layoutCols =
                static_cast<int>(ceil(selectedCount / static_cast<float>(m_editorSettings.layoutRows)));
        }
        else
        {
            if (m_editorSettings.layoutUseWidth)
            {
                m_editorSettings.layoutRows =
                    static_cast<int>(ceil(selectedCount / static_cast<float>(m_editorSettings.layoutCols)));
            }
            else
            {
                m_editorSettings.layoutCols =
                    static_cast<int>(ceil(selectedCount / static_cast<float>(m_editorSettings.layoutRows)));
            }
        }

        int i = 0;
        for (auto entity : m_selectedEntities)
        {
            auto& transform = bee::Engine.Registry().get<Transform>(entity);

            float xPos = (i % m_editorSettings.layoutCols) * m_editorSettings.layoutSpacing;
            float zPos = (i / m_editorSettings.layoutCols) * m_editorSettings.layoutSpacing;

            transform.SetPosition(glm::vec3(xPos, 0.0f, zPos));
            i++;
        }
    }

    ImGui::End();
}

std::string bee::EditorLayer::PromptForName(const std::string& popupTitle, bool openPopup)
{
    static std::string inputName;
    std::string result;

    // Open the popup if the flag is set
    if (openPopup)
    {
        ImGui::OpenPopup(popupTitle.c_str());
    }

    if (ImGui::BeginPopupModal(popupTitle.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (!ImGui::IsAnyItemActive()) ImGui::SetKeyboardFocusHere(0);

        if (ImGui::InputText("##inputName",
                             &inputName,
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            result = inputName;
            ImGui::CloseCurrentPopup();  // Close popup after name is entered
        }

        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    return result;
}


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/