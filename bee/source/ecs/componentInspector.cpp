#include "ecs/componentInspector.hpp"
#include "core.hpp"
#include "managers/particle_manager.hpp"

std::vector<entt::id_type> component_render_order = {
    entt::type_hash<bee::HierarchyNode>::value(),  // Render HierarchyNode first
    entt::type_hash<bee::Transform>::value(),
    entt::type_hash<bee::Renderable>::value(),
    entt::type_hash<bee::Emitter>::value(),
    entt::type_hash<bee::DirectionalLight>::value(),
    entt::type_hash<bee::PointLight>::value(),
    entt::type_hash<bee::SceneData>::value(),
    entt::type_hash<bee::Camera>::value()};

using namespace entt;
void bee::ComponentManager::RegisterComponents()
{
    // Register components with drawing functions
    // RegisterComponentNoDraw<UUID>("UUID", false, true);
    RegisterComponent<HierarchyNode, DrawHierarchyNodeComponent>("HierarchyNode", false, true);
    RegisterComponent<Transform, DrawTransformComponent>("Transform", false, true);
    RegisterComponent<Renderable, DrawRenderableComponent>("Renderable", true, true);
    RegisterComponent<Emitter, DrawEmitterComponent>("Emitter", true, true);
    RegisterComponent<DirectionalLight, DrawDirectionalLightComponent>("DirectionalLight", true, true);
    RegisterComponent<PointLight, DrawPointLightComponent>("PointLight", true, true);
    RegisterComponent<SceneData, DrawSceneDataComponent>("SceneData", true, true);
    RegisterComponentNoDraw<Raycastable>("Raycastable", false, true);
    RegisterComponent<Camera, DrawCameraComponent>("Camera", true, true);
    RegisterComponent<GltfScene, DrawGltfSceneComponent>("GltfScene", true, true);
    RegisterComponent<GltfNode, DrawGltfNodeComponent>("GltfNode", true, true);
    RegisterComponent<Grid, DrawTileGridComponent>("TileGrid",
                                                   true,
                                                   true,
                                                   [](entt::entity entity)
                                                   { GridManager::InitializeGrid(entity, Engine.Registry()); });
    RegisterComponentNoDraw<Cell>("Cell", false, true);
    RegisterComponentNoDraw<Disabled>("Disabled", true, true);
    RegisterComponent<Canvas, DrawCanvasComponent>("Canvas", true, true);
    RegisterComponent<CanvasElement, DrawCanvasElementComponent>("CanvasElement", true, true);

    // Register component without a drawing function
}

// Made with help from OpenAI. (2024). ChatGPT [Large language model]. https://chatgpt.com
void bee::ComponentManager::DrawComponents(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();

    // Set to track components that have already been rendered (from component_render_order)
    std::unordered_set<entt::id_type> rendered_components;

    // First, iterate over the components in the desired order
    for (entt::id_type& type_id : component_render_order)
    {
        entt::meta_type meta_type = entt::resolve(type_id);

        if (meta_type)
        {
            // Check if the entity has this component type
            if (registry.storage(type_id)->contains(entity))
            {
                // Mark this component as rendered
                rendered_components.insert(type_id);

                // Draw the component
                entt::meta_func draw_func = meta_type.func("draw"_hs);
                if (draw_func)
                {
                    draw_func.invoke({}, entity);
                }

                // Handle the "removable" property
                entt::meta_prop removable_prop = meta_type.prop("removable"_hs);
                if (removable_prop)
                {
                    bool is_removable = removable_prop.value().cast<bool>();

                    if (is_removable &&
                        ImGui::Button(((ICON_FA_TRASH TAB_FA "Remove ") + std::string(meta_type.info().name())).c_str()))
                    {
                        registry.storage(type_id)->remove(entity);
                    }
                }
                else
                {
                    std::cout << "Removable property not found for component: " << meta_type.info().name() << std::endl;
                }

                if (draw_func)
                {
                    ImGui::Separator();
                }
            }
        }
        else
        {
            std::cout << "Meta type resolution failed for ID: " << type_id << std::endl;
        }
    }

    // Render all the components that are not in the render order
    for (auto&& [component_id, storage] : registry.storage())
    {
        // Skip components that have already been rendered
        if (rendered_components.find(component_id) != rendered_components.end()) continue;

        // Check if the entity has this component
        if (storage.contains(entity))
        {
            // Resolve the meta type for this component ID
            entt::meta_type component_type = entt::resolve(component_id);

            if (component_type)
            {
                // Draw the component
                entt::meta_func draw_func = component_type.func("draw"_hs);
                if (draw_func)
                {
                    draw_func.invoke({}, entity);
                }

                // Handle the "removable" property
                entt::meta_prop removable_prop = component_type.prop("removable"_hs);
                if (removable_prop)
                {
                    bool is_removable = removable_prop.value().cast<bool>();

                    if (is_removable &&
                        ImGui::Button(((ICON_FA_TRASH TAB_FA "Remove ") + std::string(component_type.info().name())).c_str()))
                    {
                        storage.remove(entity);
                    }
                }

                if (draw_func)
                {
                    ImGui::Separator();
                }
            }
        }
    }
}

// Made with help from OpenAI. (2024). ChatGPT [Large language model]. https://chatgpt.com
void bee::ComponentManager::DrawAddComponents(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();

    if (ImGui::BeginCombo("Add Component", "Select Component"))
    {
        // Iterate over all component types in the registry
        // if an error occurs here, make sure you've registered all components
        for (auto&& [component_id, storage] : registry.storage())
        {
            // Resolve the meta type for this component ID
            entt::meta_type component_type = entt::resolve(component_id);

            // Skip if the meta type is not registered or already present on the entity
            if (!component_type || storage.contains(entity)) continue;

            // Check if the component type has a "removable" property
            if (auto removable_prop = component_type.prop("removable"_hs); removable_prop)
            {
                // Clean the type name for better display
                auto name = component_type.prop("name"_hs).value().cast<std::string>();

                // If selectable, add the component dynamically
                if (ImGui::Selectable(name.c_str()))
                {
                    // Try to construct the component (default constructor)
                    entt::meta_any instance = component_type.construct();

                    // If the construction was successful, add the component to the entity
                    if (instance)
                    {
                        // Access the storage for this component type and emplace the component
                        entt::sparse_set* storage_ptr = registry.storage(component_id);
                        storage_ptr->push(entity);  // Emplace the component in the storage

                        // Call the "onInitialize" function if it exists
                        if (auto onInitialize_prop = component_type.prop("onInitialize"_hs); onInitialize_prop)
                        {
                            auto onInitialize_func = onInitialize_prop.value().cast<std::function<void(entt::entity)>>();
                            if (onInitialize_func)
                            {
                                onInitialize_func(entity);
                            }
                        }
                    }
                }
            }
        }

        ImGui::EndCombo();
    }
}

// Helper function for beginning a tree node and ensuring it's closed
template <typename Func>
static void DrawComponent(const std::string& label, Func&& func)
{
    if (ImGui::TreeNodeEx(label.c_str(), DEFAULT_TREENODE_FLAGS))
    {
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            ImGui::OpenPopup((label + "##Popup").c_str());
        }
        func();  // Call the provided function for the specific component
        ImGui::TreePop();
    }
}

void bee::ComponentManager::DrawTransformComponent(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    auto& transform = registry.get<Transform>(entity);

    std::string label = ICON_FA_MAXIMIZE TAB_FA "Transform";
    DrawComponent(label,
                  [&]()
                  {
                      // glm::vec3 position = transform.GetPosition();
                      // glm::vec3 rotation = transform.GetRotationEuler();
                      // glm::vec3 scale = transform.GetScale();
                      if (bee::ImGuiHelper::Vec3("Position", transform.position))
                      {
                          transform.SetPosition(transform.position);
                      }
                      if (bee::ImGuiHelper::Vec3("Rotation (Euler)", transform.rotationEuler))
                      {
                          transform.SetRotation(transform.rotationEuler);
                      }
                      if (bee::ImGuiHelper::Vec3("Scale", transform.scale))
                      {
                          transform.SetScale(transform.scale);
                      }
                  });

    ComponentRightClick(label, transform);
}

void bee::ComponentManager::DrawHierarchyNodeComponent(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    auto& hierarchyNode = registry.get<HierarchyNode>(entity);

    std::string label = ICON_FA_CIRCLE_INFO TAB_FA "Hierarchy Node";
    DrawComponent(label,
                  [&]()
                  {
                      bee::ImGuiHelper::InputText("Name", hierarchyNode.name);
                      bee::ImGuiHelper::Checkbox("Select Parent On Click", &hierarchyNode.selectParent);
                  });

    ComponentRightClick(label, hierarchyNode);
}

void bee::ComponentManager::DrawRenderableComponent(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    auto& renderable = registry.get<Renderable>(entity);

    std::string label = ICON_FA_CUBE TAB_FA "Renderable";
    DrawComponent(label,
                  [&]()
                  {
                      ImGui::Text("Mesh: %d", renderable.mesh->GetHandle().id);
                      ImGui::Text("Texture: %d", renderable.texture->GetHandle().id);
                      bee::ImGuiHelper::Color("Multiplier", renderable.multiplier);
                      bee::ImGuiHelper::Color("Tint", renderable.tint);
                      bee::ImGuiHelper::Checkbox("Billboard", &renderable.billboard);
                      if (bee::ImGuiHelper::Checkbox("Visible", &renderable.visible))
                      {
                          /*                   Scope<Command> command =
                                                 CreateScope<TypeCommand<bool>>(renderable.visible, !renderable.visible,
                             renderable.visible); UndoRedoManager::Execute(std::move(command));*/
                      }
                      bee::ImGuiHelper::Checkbox("Receive Shadows", &renderable.receiveShadows);

                      if (ImGui::Button("Load OBJ File"))
                      {
                          fs::path path = bee::FileDialog::OpenFile(FILE_FILTER("Object File", ".obj"));
                          if (!path.empty())
                          {
                              renderable.mesh = bee::resource::LoadResource<bee::resource::Mesh>(path);
                              auto relative = bee::Engine.FileIO().GetRelativePath(bee::FileIO::Directory::Root, path);
                              renderable.meshPath = relative;
                          }
                      }

                      if (ImGui::Button("Load Texture File"))
                      {
                          fs::path path = bee::FileDialog::OpenFile(FILE_FILTER("Texture File", ".png"));
                          if (!path.empty())
                          {
                              renderable.texture = bee::resource::LoadResource<bee::resource::Texture>(path);
                              auto relative = bee::Engine.FileIO().GetRelativePath(bee::FileIO::Directory::Root, path);
                              renderable.texturePath = relative;
                          }
                      }
                  });

    ComponentRightClick(label, renderable);
}

void bee::ComponentManager::DrawDirectionalLightComponent(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    auto& directionalLight = registry.get<DirectionalLight>(entity);

    std::string label = ICON_FA_SUN TAB_FA "Directional Light";
    DrawComponent(label, [&]() { bee::ImGuiHelper::Color("Color", directionalLight.color); });
    ComponentRightClick(label, directionalLight);
}

void bee::ComponentManager::DrawPointLightComponent(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    auto& pointLight = registry.get<PointLight>(entity);

    std::string label = ICON_FA_LIGHTBULB TAB_FA "Point Light";
    DrawComponent(label,
                  [&]()
                  {
                      bee::ImGuiHelper::Color("Color", pointLight.color);
                      bee::ImGuiHelper::DragControl("Range", &pointLight.range);
                  });

    ComponentRightClick(label, pointLight);
}

// Additional example for a particle system component
void bee::ComponentManager::DrawEmitterComponent(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    auto& emitter = registry.get<Emitter>(entity);

    std::string label = ICON_FA_WAND_MAGIC_SPARKLES TAB_FA "Emitter";
    DrawComponent(label,
                  [&]()
                  {
                      ImGui::Text("ID: %d", emitter.id);
                      ImGui::Text("Particle Count: %s", FormatWithCommas(emitter.particleCount).c_str());

                      if (ImGui::TreeNode("Emitter Settings"))
                      {
                          bee::ImGuiHelper::Checkbox("Active", &emitter.specs.active);
                          bee::ImGuiHelper::Checkbox("Loop", &emitter.specs.loop);
                          bee::ImGuiHelper::Checkbox("Burst", &emitter.specs.burst);
                          bee::ImGuiHelper::Checkbox("World Space", &emitter.specs.useWorldSpace);
                          bee::ImGuiHelper::DragControl("Lifetime", &emitter.specs.lifetime, 0.1f);
                          bee::ImGuiHelper::DragControl("Spawn Count", &emitter.specs.spawnCountPerSecond, 0.1f);
                          bee::ImGuiHelper::DragControl("Max Particles", &emitter.specs.maxParticles, 1);
                          ImGui::TreePop();
                      }

                      if (ImGui::TreeNode("Cone Settings"))
                      {
                          bee::ImGuiHelper::Checkbox("Debug Draw", &emitter.specs.coneSpecs.debugDraw);
                          bee::ImGuiHelper::DragControl("Cone Angle", &emitter.specs.coneSpecs.angle, 0.1f, 0.0f, 180.0f);
                          ImGui::TreePop();
                      }

                      if (ImGui::TreeNode("Particle Settings"))
                      {
                          bee::ImGuiHelper::DragControl("Start Size", &emitter.particleSpecs.startSize, 0.001f);
                          bee::ImGuiHelper::DragControl("End Size", &emitter.particleSpecs.endSize, 0.001f);
                          bee::ImGuiHelper::EnumCombo("Size Ease", emitter.particleSpecs.sizeEase);

                          bee::ImGuiHelper::Checkbox("Random Start Rotation", &emitter.particleSpecs.randomStartRotation);
                          bee::ImGuiHelper::Checkbox("Randomly Rotate", &emitter.particleSpecs.randomRotate);

                          if (!emitter.particleSpecs.randomStartRotation)
                          {
                              ImGui::Indent();
                              bee::ImGuiHelper::Vec3("Start Rotation", emitter.particleSpecs.startRotation);
                              ImGui::Unindent();
                          }

                          bee::ImGuiHelper::DragControl("Rotation Speed", &emitter.particleSpecs.rotationSpeed, 0.1f);

                          bee::ImGuiHelper::Checkbox("Random Color", &emitter.particleSpecs.randomColor);
                          bee::ImGuiHelper::Checkbox("Multiply Color", &emitter.particleSpecs.multiplyColor);

                          if (emitter.particleSpecs.multiplyColor && ImGui::TreeNode("Multiply Color"))
                          {
                              ImGui::Indent();
                              emitter.particleSpecs.multiplyColorGradient.OnImGuiRender();
                              ImGui::Unindent();
                              ImGui::TreePop();
                          }

                          bee::ImGuiHelper::Checkbox("Add Color", &emitter.particleSpecs.addColor);
                          if (emitter.particleSpecs.addColor && ImGui::TreeNode("Add Color"))
                          {
                              ImGui::Indent();
                              emitter.particleSpecs.addColorGradient.OnImGuiRender();
                              ImGui::Unindent();
                              ImGui::TreePop();
                          }

                          bee::ImGuiHelper::Checkbox("Random Velocity", &emitter.particleSpecs.randomVelocity);
                          bee::ImGuiHelper::Checkbox("Draw Velocity", &emitter.particleSpecs.drawVelocity);
                          bee::ImGuiHelper::DragControl("Start Velocity", &emitter.particleSpecs.startVelocity, 0.1f);
                          bee::ImGuiHelper::Vec3("Acceleration", emitter.particleSpecs.acceleration);

                          bee::ImGuiHelper::Checkbox("Random Lifetime", &emitter.particleSpecs.randomLifetime);
                          bee::ImGuiHelper::DragControl("Start Lifetime", &emitter.particleSpecs.startLifeTime, 0.1f);
                          bee::ImGuiHelper::DragControl("Min Lifetime", &emitter.particleSpecs.minLifetime, 0.1f);
                          bee::ImGuiHelper::DragControl("Max Lifetime", &emitter.particleSpecs.maxLifetime, 0.1f);

                          ImGui::TreePop();
                      }
                  });

    ComponentRightClick(label, emitter);
}

void bee::ComponentManager::DrawSceneDataComponent(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    auto& sceneData = registry.get<SceneData>(entity);

    std::string label = ICON_FA_MOUNTAIN_SUN TAB_FA "Scene Data";
    DrawComponent(label,
                  [&]()
                  {
                      // Sky Settings
                      if (ImGui::TreeNode("Sky"))
                      {
                          sceneData.skyColor.OnImGuiRender();
                          ImGui::TreePop();
                      }

                      // Ambient Color
                      bee::ImGuiHelper::Color("Ambient Color", sceneData.ambientColor);
                  });

    ComponentRightClick(label, sceneData);
}

void bee::ComponentManager::DrawCameraComponent(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    auto& camera = registry.get<Camera>(entity);

    std::string label = ICON_FA_VIDEO TAB_FA "Camera";
    DrawComponent(label,
                  [&]()
                  {
                      bee::ImGuiHelper::Checkbox("Render", &camera.render);
                      bee::ImGuiHelper::DragControl("FOV", &camera.fov, 0.1f, 1.0f, 180.0f);
                      bee::ImGuiHelper::DragControl("Near Clip", &camera.nearClip, 0.1f, 0.0f, 100.0f);
                      bee::ImGuiHelper::DragControl("Far Clip", &camera.farClip, 0.1f, 0.0f, 1000.0f);
                      bee::ImGuiHelper::Checkbox("Orthographic", &camera.orthographic);
                      if (camera.orthographic)
                      {
                          bee::ImGuiHelper::DragControl("Orthographic Size", &camera.orthoSize, 0.1f, 0.0f, 100.0f);
                      }
                      bee::ImGuiHelper::EnumCombo("Render Mode", camera.renderMode);
                  });

    ComponentRightClick(label, camera);
}

void bee::ComponentManager::DrawGltfSceneComponent(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    auto& gltfScene = registry.get<GltfScene>(entity);

    std::string label = ICON_FA_CUBES TAB_FA "GLTF Scene";
    DrawComponent(label,
                  [&]()
                  {
                      ImGui::Text("Scene Name: %s", gltfScene.name.c_str());
        // I know this ifdef is ugly, but it's the only way to get the code to compile on both platforms, otherwise it would give a warning (warning = error)
#ifdef BEE_PLATFORM_PC
                      ImGui::Text("Path: %ls", gltfScene.path.c_str());
#else
                      ImGui::Text("Path: %s", gltfScene.path.c_str());
#endif
                  });

    ComponentRightClick(label, gltfScene);
}

void bee::ComponentManager::DrawGltfNodeComponent(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    auto& gltfNode = registry.get<GltfNode>(entity);

    std::string label = "GLTF Node";
    DrawComponent(label,
                  [&]()
                  {
                      ImGui::Text("Node: %d", gltfNode.meshId);
                      ImGui::Text("Node Name: %s", gltfNode.name.c_str());
                  });

    ComponentRightClick(label, gltfNode);
}

void bee::ComponentManager::DrawTileGridComponent(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    auto& tileGrid = registry.get<Grid>(entity);

    std::string label = ICON_FA_BORDER_ALL TAB_FA "Tile Grid";
    DrawComponent(label,
                  [&]()
                  {
                      bool changed = false;
                      changed |= ImGui::DragInt("Width", &tileGrid.width, 1);
                      changed |= ImGui::DragInt("Height", &tileGrid.height, 1);
                      changed |= ImGui::DragFloat("Tile Size", &tileGrid.tileSize, 0.01f);
                      changed |= ImGui::DragFloat("Spacing", &tileGrid.spacing, 0.01f);
                      ImGui::Checkbox("Show Grid", &tileGrid.showGrid);

                      if (changed)
                      {
                          GridManager::ResizeGrid(entity, registry);
                      }
                  });

    ComponentRightClick(label, tileGrid);
}

void bee::ComponentManager::DrawCanvasComponent(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    auto& canvas = registry.get<Canvas>(entity);

    std::string label = ICON_FA_PAINT_BRUSH TAB_FA "Canvas";
    DrawComponent(label, [&]() {});

    ComponentRightClick(label, canvas);
}

void bee::ComponentManager::DrawCanvasElementComponent(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    auto& canvasElement = registry.get<CanvasElement>(entity);

    std::string label = ICON_FA_PAINT_BRUSH TAB_FA "Canvas Element";
    DrawComponent(label, [&]() { bee::ImGuiHelper::EnumCombo("Anchor", canvasElement.anchor); });

    ComponentRightClick(label, canvasElement);
}

// Made with help from OpenAI. (2024). ChatGPT [Large language model]. https://chatgpt.com
//   Helper function to clean the type name
std::string bee::ComponentManager::CleanTypeName(const std::string& type_name)
{
    std::string cleaned_name = type_name;

    // Remove "struct " if present
    const std::string struct_prefix = "struct ";
    size_t struct_pos = cleaned_name.find(struct_prefix);
    if (struct_pos != std::string::npos)
    {
        cleaned_name.erase(struct_pos, struct_prefix.length());
    }

    // Remove any namespace prefix like "bee::"
    const std::string ns_prefix = "bee::";
    size_t ns_pos = cleaned_name.find(ns_prefix);
    if (ns_pos != std::string::npos)
    {
        cleaned_name.erase(ns_pos, ns_prefix.length());
    }

    // Remove any trailing "(void) noexcept" or similar
    size_t paren_pos = cleaned_name.find('(');
    if (paren_pos != std::string::npos)
    {
        cleaned_name.erase(paren_pos);
    }

    // Remove trailing angle bracket like '>' from templates
    size_t angle_bracket_pos = cleaned_name.find('>');
    if (angle_bracket_pos != std::string::npos)
    {
        cleaned_name.erase(angle_bracket_pos);
    }

    // Trim any extra spaces (just in case)
    cleaned_name.erase(0, cleaned_name.find_first_not_of(" \n\r\t"));
    cleaned_name.erase(cleaned_name.find_last_not_of(" \n\r\t") + 1);

    return cleaned_name;
}