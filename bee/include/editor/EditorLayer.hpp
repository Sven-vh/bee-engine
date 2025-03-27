#pragma once
#include "core.hpp"
#include "editor/AssetBrowser.hpp"

namespace bee
{

enum CamType
{
    Freefly,
    Orbit,
    Up,
    Right,
    Forward
};

struct EditorSettings
{
    bool m_showToolbar = true;
    bool m_showHitboxes = false;
    bool m_showIcons = true;
    bool m_showCameras = true;

    float m_positionRoundingValue = 0.5f;
    float m_rotationRoundingValue = 5.0f;
    float m_scaleRoundingValue = 0.1f;

    CamType m_currentCamType = CamType::Freefly;

    float m_cameraSpeed = 10.0f;
    float m_cameraSensitivity = 0.1f;

    bool showProfiler = false;
    bool showResourceManager = false;
    bool showLayoutWindow = false;
    bool showUndoRedoManager = false;
    bool showRenderStats = false;
    bool showRegistryStats = false;
    bool showAssetsBrowser = false;

    bool layoutAutoSize = true;
    bool layoutUseWidth = false;
    int layoutRows = 1;
    int layoutCols = 1;
    float layoutSpacing = 3.0f;

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(m_showToolbar,
                m_showHitboxes,
                m_showIcons,
                m_showCameras,
                m_positionRoundingValue,
                m_rotationRoundingValue,
                m_scaleRoundingValue,
                m_currentCamType,
                m_cameraSpeed,
                m_cameraSensitivity,
                showProfiler,
                showResourceManager,
                showLayoutWindow,
                showUndoRedoManager,
                showRenderStats,
                showRegistryStats,
                showAssetsBrowser,
                layoutAutoSize,
                layoutUseWidth,
                layoutRows,
                layoutCols,
                layoutSpacing);
    }
};

class EditorLayer : public bee::Layer
{
public:
    EditorLayer();
    ~EditorLayer();

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnRender() override;
    virtual void OnImGuiRender() override;
    virtual void OnUpdate(float deltaTime) override;
    virtual void OnFixedUpdate(float deltaTime) override;

    virtual void OnEvent(Event& e) override;

    virtual void Begin(const entt::entity cameraEntity);
    virtual void End(const entt::entity cameraEntity);

private:
    AssetBrowser m_assetBrowser;

    EditorSettings m_editorSettings;

    std::map<entt::entity, Ref<FrameBuffer>> m_frameBuffers;

    entt::entity m_lastSelectedEntity;
    std::set<entt::entity> m_selectedEntities;
    std::set<entt::entity> m_pinnedCameras;
    ImGuizmo::OPERATION m_currentGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
    ImGuizmo::MODE m_currentGizmoMode = ImGuizmo::MODE::WORLD;

    bool m_viewportEditorFocused = false;
    bool m_viewportGameFocused = false;
    glm::vec2 m_viewportSize = {0.0f, 0.0f};
    glm::vec2 m_viewportPosition = {0.0f, 0.0f};

    bool m_editorSettingsWindow = false;
    bool m_cameraSettingsWindow = false;
    bool m_renameWindow = false;
    bool m_openSaveAndPlayPopup = false;
    bool m_sceneChangedPopup = false;
    bool m_savePrefabPopup = false;

    raycasting::Ray m_previousRay;
    fs::path m_loadedSceneName = "";

private:
    static void MainDockSpace(const ImGuiDockNodeFlags dockspace_flags);
    static void ApplyStyle(const bool opt_fullscreen, const ImGuiDockNodeFlags dockspace_flags, bool& dockspaceOpen);
    void MenuBar();
    void Viewport(const std::string& windowName, const entt::entity cameraEntity, bool showToolbar);
    void UpdateCameraConfig(const entt::entity cameraEntity) const;
    void SceneHierarchy();
    void Gizmos(entt::entity camera);
    void Inspector();
    void DrawRaycastables() const;
    void DrawIcons() const;
    void DrawCameras() const;
    static void DrawGrids();
    void ShowRegistryStats();

    void LayoutEditor();

    void DrawEntityNode(entt::entity entity);
    void SelectEntitiesInRange(entt::entity first, entt::entity last);

    bool OnKeyPressed(KeyPressedEvent& e);
    bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

    bool CheckGridIntersections();
    void UpdateGrids();
    void CheckEntityIntersections();

    void SaveScene();
    void SaveSceneAs(std::string& saveName);

    void DrawEditorCamera();
    void MoveEditorCamera(float deltatime);

    glm::vec3 GetAveragePositionOfSelectedEntities();
    static std::string PromptForName(const std::string& popupTitle, bool openPopup);
};
}  // namespace bee


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/