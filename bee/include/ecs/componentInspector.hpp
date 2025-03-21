#pragma once
#include "common.hpp"
#include "ecs/components.hpp"
#include "ecs/enttCereal.hpp"
#include "managers/undo_redo_manager.hpp"
#include "tools/imguiHelper.hpp"

#define DEFAULT_TREENODE_FLAGS \
    (ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth)

namespace bee
{
class ComponentManager
{
public:
    static void RegisterComponents();
    static void DrawComponents(entt::entity entity);
    static void DrawAddComponents(entt::entity entity);

    template <typename ComponentType, auto DrawFunction>
    static void RegisterComponent(std::string name,
                                  bool removable,
                                  bool serializable,
                                  std::function<void(entt::entity)> initializeFunction = nullptr);

    template <typename ComponentType, auto DrawFunction>
    static void RegisterComponentNoSerialize(std::string name, bool removable);

    template <typename ComponentType>
    static void RegisterComponentNoDraw(std::string name, bool removable, bool serializable);

    static std::string CleanTypeName(const std::string& type_name);

private:
    static void DrawTransformComponent(entt::entity entity);
    static void DrawHierarchyNodeComponent(entt::entity entity);
    static void DrawRenderableComponent(entt::entity entity);
    static void DrawEmitterComponent(entt::entity entity);
    static void DrawDirectionalLightComponent(entt::entity entity);
    static void DrawPointLightComponent(entt::entity entity);
    static void DrawSceneDataComponent(entt::entity entity);
    static void DrawCameraComponent(entt::entity entity);
    static void DrawGltfSceneComponent(entt::entity entity);
    static void DrawGltfNodeComponent(entt::entity entity);
    static void DrawTileGridComponent(entt::entity entity);
    static void DrawCanvasComponent(entt::entity entity);
    static void DrawCanvasElementComponent(entt::entity entity);

    template <typename ComponentType>
    static void ComponentRightClick(const std::string& label, ComponentType& transform);
};

template <typename ComponentType, auto DrawFunction>
inline void ComponentManager::RegisterComponent(std::string name,
                                                bool removable,
                                                bool serializable,
                                                std::function<void(entt::entity)> initializeFunction)
{
    using namespace entt;

    // Register component with meta and drawing function
    entt::meta<ComponentType>()
        .type(entt::type_hash<ComponentType>::value())
        .prop("removable"_hs, removable)
        .prop("name"_hs, name)
        .prop("onInitialize"_hs, initializeFunction)
#ifdef BEE_PLATFORM_PC
        .func<DrawFunction>("draw"_hs)
#endif
        ;

    // Register the component for serialization if required
    if (serializable)
    {
        registerComponentForSerialization<ComponentType, cereal::JSONOutputArchive>(name);
        registerComponentForDeserialization<ComponentType, cereal::JSONInputArchive>(name);
    }

    // create an entity with the component, and then delete it so that the component is registered
    entt::entity entity = Engine.Registry().create();
    Engine.Registry().emplace<ComponentType>(entity);
    Engine.Registry().destroy(entity);
}

template <typename ComponentType, auto DrawFunction>
inline void ComponentManager::RegisterComponentNoSerialize(std::string name, bool removable)
{
    using namespace entt;

    // Register component with meta and drawing function
    entt::meta<ComponentType>()
        .type(entt::type_hash<ComponentType>::value())
        .prop("removable"_hs, removable)
        .prop("name"_hs, name)
        .func<DrawFunction>("draw"_hs);
    // create an entity with the component, and then delete it so that the component is registered
    entt::entity entity = Engine.Registry().create();
    Engine.Registry().emplace<ComponentType>(entity);
    Engine.Registry().destroy(entity);
}

template <typename ComponentType>
inline void ComponentManager::RegisterComponentNoDraw(std::string name, bool removable, bool serializable)
{
    using namespace entt;

    // Register component with meta but without drawing function
    entt::meta<ComponentType>()
        .type(entt::type_hash<ComponentType>::value())
        .prop("removable"_hs, removable)
        .prop("name"_hs, name);

    // Register the component for serialization if required
    if (serializable)
    {
        registerComponentForSerialization<ComponentType, cereal::JSONOutputArchive>(name);
        registerComponentForDeserialization<ComponentType, cereal::JSONInputArchive>(name);
    }

    // create an entity with the component, and then delete it so that the component is registered
    entt::entity entity = Engine.Registry().create();
    Engine.Registry().emplace<ComponentType>(entity);
    Engine.Registry().destroy(entity);
}

template <typename ComponentType>
inline void ComponentManager::ComponentRightClick(const std::string& label, ComponentType& component)
{
    ImGui::PushID(label.c_str());

    if (ImGui::BeginPopup((label + std::string("##Popup")).c_str()))
    {
        if (ImGui::MenuItem("Copy"))
        {
            ImGuiHelper::CopyToClipboard(component);
        }
        if (ImGui::MenuItem("Paste"))
        {
            ImGuiHelper::PasteFromClipboard(component);
        }
        if (ImGui::MenuItem("Reset"))
        {
            component = ComponentType();
        }
        ImGui::EndPopup();
    }

    ImGui::PopID();
}
}  // namespace bee