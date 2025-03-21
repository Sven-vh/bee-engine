#include "ecs/componentInitialize.hpp"
#include "core.hpp"

entt::entity bee::ecs::CreateEmpty()
{
    entt::registry& registry = bee::Engine.Registry();
    entt::entity entity = registry.create();
    return entity;
}

entt::entity bee::ecs::CreateDefault(const std::string& name)
{
    entt::registry& registry = bee::Engine.Registry();
    entt::entity entity = CreateEmpty();
    registry.emplace<HierarchyNode>(entity);
    registry.get<HierarchyNode>(entity).name = name;
    registry.emplace<Transform>(entity);
    registry.emplace<Raycastable>(entity);
    registry.emplace<Saveable>(entity);
    return entity;
}

entt::entity bee::ecs::CreateMesh()
{
    entt::registry& registry = bee::Engine.Registry();
    entt::entity entity = CreateDefault(ICON_FA_CUBE SPACE_FA "3D Mesh");
    registry.emplace<Renderable>(entity);
    return entity;
}

entt::entity bee::ecs::CreateDirectionalLight()
{
    entt::registry& registry = bee::Engine.Registry();
    entt::entity entity = CreateDefault(ICON_FA_SUN SPACE_FA "Directional Light");
    registry.emplace<DirectionalLight>(entity);
    registry.emplace<EditorIcon>(entity, "icons/Directional_Light.png");
    return entity;
}

entt::entity bee::ecs::CreatePointLight()
{
    entt::registry& registry = bee::Engine.Registry();
    entt::entity entity = CreateDefault(ICON_FA_LIGHTBULB SPACE_FA "Point Light");
    registry.emplace<PointLight>(entity);
    return entity;
}

entt::entity bee::ecs::CreateEmitter()
{
    entt::registry& registry = bee::Engine.Registry();
    entt::entity entity = CreateDefault(ICON_FA_WAND_MAGIC_SPARKLES SPACE_FA "Emitter");
    registry.emplace<Emitter>(entity);
    registry.emplace<EditorIcon>(entity, "icons/Emitter.png");
    return entity;
}

entt::entity bee::ecs::CreateSceneData()
{
    entt::registry& registry = bee::Engine.Registry();

    // check if scene data already exists
    auto sceneData = registry.view<SceneData>();
    if (!sceneData.empty())
    {
        return sceneData.front();
    }

    entt::entity entity = CreateDefault(ICON_FA_MOUNTAIN_SUN SPACE_FA "Scene Data");
    registry.emplace<SceneData>(entity);
    // remove the raycastable component
    registry.remove<Raycastable>(entity);
    return entity;
}

entt::entity bee::ecs::CreateCamera()
{
    entt::registry& registry = bee::Engine.Registry();
    entt::entity entity = CreateDefault(ICON_FA_VIDEO SPACE_FA "Camera");
    registry.emplace<Camera>(entity, Camera(45.0f, 1920.0f / 1080.0f, 0.1f, 100.0f));
    registry.emplace<EditorIcon>(entity, "icons/Camera.png");
    return entity;
}

entt::entity bee::ecs::CreateGrid()
{
    entt::registry& registry = bee::Engine.Registry();
    entt::entity entity = CreateDefault(ICON_FA_BORDER_ALL SPACE_FA "Grid");
    registry.emplace<Grid>(entity);
    GridManager::InitializeGrid(entity, registry);
    return entity;
}

entt::entity bee::ecs::CreateCanvas()
{
    entt::registry& registry = bee::Engine.Registry();
    entt::entity entity = CreateCamera();
    HierarchyNode& node = registry.get<HierarchyNode>(entity);
    node.name = ICON_FA_DIAGRAM_LEAN_CANVAS SPACE_FA "Canvas";
    registry.emplace<Canvas>(entity);
    Camera& camera = registry.get<Camera>(entity);
    camera.orthographic = true;
    camera.nearClip = -1.0f;
    return entity;
}

entt::entity bee::ecs::CreateCanvasElement()
{
    entt::registry& registry = bee::Engine.Registry();
    entt::entity entity = CreateDefault(ICON_FA_IMAGE SPACE_FA "Image");
    registry.emplace<CanvasElement>(entity);
    registry.emplace<Renderable>(entity);
    Renderable& renderable = registry.get<Renderable>(entity);
    renderable.receiveShadows = false;
    fs::path path = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_QUAD);
    renderable.mesh = bee::resource::LoadResource<bee::resource::Mesh>(path);
    renderable.meshPath = path;
    return entity;
}