#pragma once
#ifndef BEE_ENTT_HELPER_HPP
#define BEE_ENTT_HELPER_HPP

#include "common.hpp"
#include "ecs/components.hpp"

namespace bee::ecs
{

void DuplicateEntityRecursive(entt::registry& registry,
                              entt::entity sourceEntity,
                              std::unordered_map<entt::entity, entt::entity>& entityMap);
entt::entity DuplicateEntity(entt::registry& registry, entt::entity sourceEntity);
void DuplicateSelectedEntities(entt::registry& registry,
                               const std::set<entt::entity>& selectedEntities,
                               std::set<entt::entity>& newSelectedEntities);
void DestroyEntity(entt::entity& entity, entt::registry& registry);
void SetParentChildRelationship(entt::entity child,
                                entt::entity newParent,
                                entt::registry& registry,
                                bool resetTransform = false);
void RemoveParentChildRelationship(entt::entity child, entt::registry& registry);
void UpdateEntityMapping(std::unordered_map<entt::entity, entt::entity>& entity_mapping, entt::registry& registry);
void MarkAllAsDirty(entt::registry& registry);

entt::entity GetUppestParent(entt::registry& registry, entt::entity entity);
entt::entity GetUppestSelectableParent(entt::registry& registry, entt::entity entity);
std::vector<entt::entity> GetAllParents(entt::registry& registry, entt::entity entity);
std::vector<entt::entity> GetAllChildren(entt::registry& registry, entt::entity entity);
std::vector<entt::entity> FindEntitiesByName(entt::registry& registry, const std::string& name);

void RenderEntity(entt::registry& registry, entt::entity entity, const Ref<bee::FrameBuffer>& frameBuffer);

template <typename Component>
std::vector<entt::entity> GetAllChildrenWithComponent(entt::registry& registry, entt::entity entity)
{
    std::vector<entt::entity> childrenWithComponent;

    for (auto child : GetAllChildren(registry, entity))
    {
        if (registry.all_of<Component>(child))
        {
            childrenWithComponent.push_back(child);
        }
    }
    return childrenWithComponent;
}

template <typename Component>
std::vector<entt::entity> GetAllParentsWithComponent(entt::registry& registry, entt::entity entity)
{
    std::vector<entt::entity> parentsWithComponent;

    for (auto parent : GetAllParents(registry, entity))
    {
        if (registry.all_of<Component>(parent))
        {
            parentsWithComponent.push_back(parent);
        }
    }
    return parentsWithComponent;
}

template <typename Component>
entt::entity GetUppestParentWithComponent(entt::registry& registry, entt::entity entity)
{
    auto parents = GetAllParents(registry, entity);
    for (auto it = parents.rbegin(); it != parents.rend(); ++it)
    {
        if (registry.all_of<Component>(*it))
        {
            return *it;
        }
    }
    return entt::null;
}

template <typename... Component>
std::vector<entt::entity> GetEntitiesWithComponent(const entt::registry& registry)
{
    std::vector<entt::entity> entities;
    auto view = registry.view<Component...>();

    for (auto entity : view)
    {
        entities.push_back(entity);
    }
    return entities;
}

}  // namespace bee::ecs

#endif  // BEE_ENTT_HELPER_HPP


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/