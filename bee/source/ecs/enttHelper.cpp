#include "ecs/enttHelper.hpp"
#include "core.hpp"

void bee::ecs::DuplicateEntityRecursive(entt::registry& registry,
                                        entt::entity sourceEntity,
                                        std::unordered_map<entt::entity, entt::entity>& entityMap)
{
    if (entityMap.find(sourceEntity) != entityMap.end()) return;  // Already duplicated

    // Create a new entity in the registry
    entt::entity newEntity = ecs::CreateEmpty();
    entityMap[sourceEntity] = newEntity;

    for (auto [id, storage] : registry.storage())
    {
        if (storage.contains(sourceEntity))
        {
            storage.push(newEntity, storage.value(sourceEntity));
        }
    }

    // Handle HierarchyNode separately due to its parent-child structure
    if (registry.any_of<bee::HierarchyNode>(sourceEntity))
    {
        auto& oldNode = registry.get<bee::HierarchyNode>(sourceEntity);

        // Recursively duplicate children
        for (auto oldChild : oldNode.children)
        {
            DuplicateEntityRecursive(registry, oldChild, entityMap);
        }
    }

    // add the entity to the parent children list
    if (registry.any_of<bee::HierarchyNode>(newEntity))
    {
        auto& newNode = registry.get<bee::HierarchyNode>(newEntity);
        if (newNode.parent != entt::null)
        {
            auto& parentNode = registry.get<bee::HierarchyNode>(newNode.parent);
            parentNode.children.push_back(newEntity);
        }
    }

    // emitter id
    if (registry.any_of<Emitter>(newEntity))
    {
        auto& newEmitter = registry.get<Emitter>(newEntity);

        newEmitter.id = rand();
    }
}

entt::entity bee::ecs::DuplicateEntity(entt::registry& registry, entt::entity sourceEntity)
{
    std::unordered_map<entt::entity, entt::entity> entityMap = {};
    DuplicateEntityRecursive(registry, sourceEntity, entityMap);
    return entityMap[sourceEntity];
};

void bee::ecs::DuplicateSelectedEntities(entt::registry& registry,
                                         const std::set<entt::entity>& selectedEntities,
                                         std::set<entt::entity>& newSelectedEntities)
{
    std::unordered_map<entt::entity, entt::entity> entityMap;

    // Duplicate each selected entity and its children recursively
    for (auto entity : selectedEntities)
    {
        DuplicateEntityRecursive(registry, entity, entityMap);
    }

    // After duplication, handle special cases and update parent-child relationships
    for (const auto& [oldEntity, newEntity] : entityMap)
    {
        // Handle specific cases like resetting or assigning unique values
        if (registry.any_of<Emitter>(newEntity))
        {
            auto& emitter = registry.get<Emitter>(newEntity);
            emitter.id = rand();        // Assign a new unique ID
            emitter.particleCount = 0;  // Reset particle count
        }

        if (registry.all_of<bee::HierarchyNode>(oldEntity))
        {
            auto& oldNode = registry.get<bee::HierarchyNode>(oldEntity);
            auto& newNode = registry.get<bee::HierarchyNode>(newEntity);

            // Update parent if it exists and has been duplicated
            if (oldNode.parent != entt::null)
            {
                if (entityMap.count(oldNode.parent))
                {
                    newNode.parent = entityMap[oldNode.parent];
                }
                else
                {
                    newNode.parent = oldNode.parent;
                }
                auto& parentNode = registry.get<bee::HierarchyNode>(newNode.parent);

                // Avoid adding duplicate child entities to the parent's children vector
                if (std::find(parentNode.children.begin(), parentNode.children.end(), newEntity) == parentNode.children.end())
                {
                    parentNode.children.push_back(newEntity);
                }
            }

            // Update children references for the duplicated node
            newNode.children.clear();  // Clear any existing children references
            for (auto oldChild : oldNode.children)
            {
                if (entityMap.count(oldChild))
                {
                    newNode.children.push_back(entityMap[oldChild]);
                }
            }
        }
    }

    // Clear the new selected entities and populate with new duplicates
    newSelectedEntities.clear();
    for (const auto& [oldEntity, newEntity] : entityMap)
    {
        newSelectedEntities.insert(newEntity);
    }
}

entt::entity bee::ecs::GetUppestParent(entt::registry& registry, entt::entity entity)
{
    auto& node = registry.get<HierarchyNode>(entity);
    if (node.parent == entt::null)
    {
        return entity;
    }
    return GetUppestParent(registry, node.parent);
}

entt::entity bee::ecs::GetUppestSelectableParent(entt::registry& registry, entt::entity entity)
{
    if (registry.all_of<HierarchyNode>(entity) == false)
    {
        return entt::null;
    }
    auto& node = registry.get<HierarchyNode>(entity);
    if (node.parent == entt::null)
    {
        return entity;
    }
    // its true if the parent should be selected
    if (node.selectParent)
    {
        if (node.parent != entt::null)
        {
            return GetUppestSelectableParent(registry, node.parent);
        }
        else
        {
            return entity;
        }
    }
    else
    {
        return entity;
    }
}

std::vector<entt::entity> bee::ecs::GetAllParents(entt::registry& registry, entt::entity entity)
{
    std::vector<entt::entity> parents;
    auto& node = registry.get<HierarchyNode>(entity);
    if (node.parent == entt::null)
    {
        return parents;
    }

    parents.push_back(node.parent);
    auto parentParents = GetAllParents(registry, node.parent);
    parents.insert(parents.end(), parentParents.begin(), parentParents.end());
    return parents;
}

std::vector<entt::entity> bee::ecs::GetAllChildren(entt::registry& registry, entt::entity entity)
{
    std::vector<entt::entity> children;
    if (!registry.all_of<HierarchyNode>(entity))
    {
        return children;
    }
    auto& node = registry.get<HierarchyNode>(entity);
    for (auto child : node.children)
    {
        children.push_back(child);
        auto childChildren = GetAllChildren(registry, child);
        children.insert(children.end(), childChildren.begin(), childChildren.end());
    }
    return children;
}

std::vector<entt::entity> bee::ecs::FindEntitiesByName(entt::registry& registry, const std::string& name)
{
    std::vector<entt::entity> entities;
    auto view = registry.view<HierarchyNode>();
    for (auto entity : view)
    {
        auto& node = view.get<HierarchyNode>(entity);
        if (node.name == name)
        {
            entities.push_back(entity);
        }
    }

    if (entities.empty())
    {
        bee::Log::Warn("No entities found with name {}", name);
    }

    return entities;
}

void bee::ecs::DestroyEntity(entt::entity& entity, entt::registry& registry)
{
    if (!registry.valid(entity))
    {
        return;
    }
    auto& node = registry.get<HierarchyNode>(entity);
    auto& parent = node.parent;
    if (parent != entt::null)
    {
        auto& parentNode = registry.get<HierarchyNode>(parent);
        parentNode.children.erase(std::remove(parentNode.children.begin(), parentNode.children.end(), entity),
                                  parentNode.children.end());
    }
    node.DestroyChildren(registry);
    registry.destroy(entity);
}

void bee::ecs::SetParentChildRelationship(entt::entity child,
                                          entt::entity newParent,
                                          entt::registry& registry,
                                          bool resetTransform)
{
    // Get the current node and the new parent node
    auto& childNode = registry.get<HierarchyNode>(child);
    glm::mat4 originalWorldTransform = GetWorldModel(child, registry);
    auto& parentNode = registry.get<HierarchyNode>(newParent);

    // Remove the child from its current parent if it has one
    if (childNode.parent != entt::null)
    {
        auto& oldParentNode = registry.get<HierarchyNode>(childNode.parent);
        oldParentNode.children.erase(std::remove(oldParentNode.children.begin(), oldParentNode.children.end(), child),
                                     oldParentNode.children.end());
    }

    // Set the new parent
    childNode.parent = newParent;
    parentNode.children.push_back(child);
    if (!resetTransform) SetWorldModel(child, originalWorldTransform, registry);
}

void bee::ecs::RemoveParentChildRelationship(entt::entity child, entt::registry& registry)
{
    auto& childNode = registry.get<HierarchyNode>(child);
    if (childNode.parent != entt::null)
    {
        auto& parentNode = registry.get<HierarchyNode>(childNode.parent);
        parentNode.children.erase(std::remove(parentNode.children.begin(), parentNode.children.end(), child),
                                  parentNode.children.end());
        childNode.parent = entt::null;
    }

    // Remove the child from its parent's children list
    if (registry.any_of<bee::HierarchyNode>(childNode.parent))
    {
        auto& parent = registry.get<bee::HierarchyNode>(childNode.parent);
        parent.children.erase(std::remove(parent.children.begin(), parent.children.end(), child), parent.children.end());
    }
}

void bee::ecs::UpdateEntityMapping(std::unordered_map<entt::entity, entt::entity>& entity_mapping, entt::registry& registry)
{
    for (auto&& [old_entity, new_entity] : entity_mapping)
    {
        if (auto* node = registry.try_get<bee::HierarchyNode>(new_entity))
        {
            if (node->parent != entt::null && entity_mapping.find(node->parent) != entity_mapping.end())
            {
                node->parent = entity_mapping[node->parent];
            }

            for (auto& child : node->children)
            {
                if (entity_mapping.find(child) != entity_mapping.end())
                {
                    child = entity_mapping[child];
                }
            }
        }
    }

    // loop through Hierarchy Node in the registry and update the entity mapping
    for (auto&& [entity, node] : registry.view<bee::HierarchyNode>().each())
    {
        if (node.parent != entt::null && entity_mapping.find(node.parent) != entity_mapping.end())
        {
            node.parent = entity_mapping[node.parent];
        }

        for (auto& child : node.children)
        {
            if (entity_mapping.find(child) != entity_mapping.end())
            {
                child = entity_mapping[child];
            }
        }
    }

    // check for every new entity its parent to see if the the parent has the child in its list
    for (auto&& [old_entity, new_entity] : entity_mapping)
    {
        if (auto* node = registry.try_get<bee::HierarchyNode>(new_entity))
        {
            if (node->parent != entt::null)
            {
                auto& parent_node = registry.get<bee::HierarchyNode>(node->parent);
                if (std::find(parent_node.children.begin(), parent_node.children.end(), new_entity) ==
                    parent_node.children.end())
                {
                    parent_node.children.push_back(new_entity);
                }
            }
        }
    }
}

void bee::ecs::MarkAllAsDirty(entt::registry& registry)
{
    registry.view<bee::Transform>().each([](auto& transform) { transform.SetDirty(true); });
}

// BoundingInfo Structure
struct BoundingInfo
{
    glm::vec3 minBounds;
    glm::vec3 maxBounds;
    glm::vec3 center;
};

// CameraMatrices Structure
struct CameraMatrices
{
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};

// Function to Compute Bounding Box and Center
BoundingInfo ComputeBoundingBoxAndCenter(entt::entity entity, entt::registry& registry)
{
    BoundingInfo info;
    info.minBounds = glm::vec3(FLT_MAX);
    info.maxBounds = glm::vec3(FLT_MIN);
    info.center = glm::vec3(0.0f);
    int count = 0;

    auto children = bee::ecs::GetAllChildren(registry, entity);
    children.push_back(entity);
    for (auto child : children)
    {
        if (!registry.all_of<bee::Renderable>(child)) continue;
        const auto& renderable = registry.get<bee::Renderable>(child);

        // Calculate mesh bounds
        glm::vec3 meshMin = renderable.mesh->GetHandle().meshCenter - renderable.mesh->GetHandle().meshSize * 0.5f;
        glm::vec3 meshMax = renderable.mesh->GetHandle().meshCenter + renderable.mesh->GetHandle().meshSize * 0.5f;

        info.minBounds = glm::min(info.minBounds, meshMin);
        info.maxBounds = glm::max(info.maxBounds, meshMax);
        info.center += renderable.mesh->GetHandle().meshCenter;
        ++count;
    }

    if (count > 0)
    {
        info.center /= static_cast<float>(count);
    }

    return info;
}

// Function to Setup Camera Matrices
CameraMatrices SetupCameraMatrices(const BoundingInfo& info)
{
    glm::vec3 size = info.maxBounds - info.minBounds;
    float maxDimension = glm::max(size.x, glm::max(size.y, size.z));
    float distance = maxDimension * 2.0f;  // Increased distance for better visibility

    // Define the size of the orthographic box
    float paddingFactor = 1.6f;  // Default value, can be adjusted
    float orthoSize = (maxDimension / 2.0f) * paddingFactor;

    // Calculate aspect ratio (assuming square framebuffer as in original code)
    float aspectRatio = 1.0f;  // Modify if framebuffer is not square

    // Calculate left, right, bottom, top based on orthoSize and aspect ratio
    float left = -orthoSize * aspectRatio;
    float right = orthoSize * aspectRatio;
    float bottom = -orthoSize;
    float top = orthoSize;

    CameraMatrices matrices;

    // Setup view matrix
    matrices.viewMatrix = glm::lookAt(glm::vec3(info.center.x + distance, info.center.y + distance, info.center.z + distance),
                                      info.center,
                                      glm::vec3(0.0f, 1.0f, 0.0f));

    // Setup orthographic projection matrix
    matrices.projectionMatrix = glm::ortho(left,
                                           right,
                                           bottom,
                                           top,
                                           0.1f,                           // Near plane
                                           distance + maxDimension * 3.0f  // Far plane
    );

    return matrices;
}

void bee::ecs::RenderEntity(entt::registry& registry, entt::entity entity, const Ref<bee::FrameBuffer>& frameBuffer)
{
    // Bind the framebuffer
    frameBuffer->Bind();
    frameBuffer->Clear();

    // Compute bounding information
    BoundingInfo boundingInfo = ComputeBoundingBoxAndCenter(entity, registry);
    CameraMatrices camera = SetupCameraMatrices(boundingInfo);

    xsr::clear_entries();

    // Render all renderable children
    auto children = bee::ecs::GetAllChildren(registry, entity);
    children.push_back(entity);
    for (auto child : children)
    {
        if (!registry.all_of<bee::Renderable>(child)) continue;
        const auto& renderable = registry.get<bee::Renderable>(child);
        glm::mat4 model = bee::GetWorldModel(child, registry);

        xsr::render_mesh(glm::value_ptr(model),
                         renderable.mesh->GetHandle(),
                         renderable.texture->GetHandle(),
                         glm::value_ptr(glm::vec4(1.0f)),
                         glm::value_ptr(glm::vec4(0.0f)),
                         true);
    }

    bee::Transform lightTransform;
    lightTransform.SetRotation(glm::vec3(-60.0f, 50.0f, 0.0f));
    auto color = glm::vec4(glm::vec3(1.0f), 0.25f);
    xsr::render_directional_light(glm::value_ptr(lightTransform.GetDirection()), glm::value_ptr(color));
    xsr::render_ambient_light(glm::value_ptr(color));

    // Render the scene (e.g., grid)
    bool gridCache = xsr::get_grid_settings().showGrid;
    xsr::get_grid_settings().showGrid = false;
    xsr::render(glm::value_ptr(camera.viewMatrix), glm::value_ptr(camera.projectionMatrix));
    xsr::clear_entries();
    xsr::get_grid_settings().showGrid = gridCache;

    // Unbind the framebuffer
    frameBuffer->Unbind();
}


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/