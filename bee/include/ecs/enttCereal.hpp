#pragma once
#include "common.hpp"
#include "ecs/components.hpp"
#include "core/engine.hpp"
#include "managers/grid_manager.hpp"
#include "ecs/enttHelper.hpp"

// Typedef for archive functions
template <typename Archive>
using SerializeFunction = std::function<void(entt::registry&, entt::entity, Archive&)>;

template <typename Archive>
using DeserializeFunction = std::function<void(entt::registry&, entt::entity, Archive&)>;

// Maps for dynamically registered serialization and deserialization functions
template <typename Archive>
std::map<std::string, SerializeFunction<Archive>> serialize_functions;

template <typename Archive>
std::map<std::string, DeserializeFunction<Archive>> deserialize_functions;

namespace bee
{
template <typename Component, typename Archive>
void registerComponentForSerialization(std::string componentName)
{
    serialize_functions<Archive>[componentName] =
        [componentName](entt::registry& registry, entt::entity entity, Archive& archive)
    {
        if (registry.all_of<Component>(entity))
        {
            // archive(cereal::make_nvp("hasComponent_" + componentName, true));
            make_optional_nvp(archive, ("hasComponent_" + componentName).c_str(), true);
            // archive(cereal::make_nvp(componentName.c_str(), registry.get<Component>(entity)));
            make_optional_nvp(archive, componentName.c_str(), registry.get<Component>(entity));
            // bee::Log::Info("Serialized component ", componentName);
        }
        else
        {
            // archive(cereal::make_nvp("hasComponent_" + componentName, false));
            make_optional_nvp(archive, ("hasComponent_" + componentName).c_str(), false);
        }
    };
}

template <typename Component, typename Archive>
void registerComponentForDeserialization(std::string componentName)
{
    deserialize_functions<Archive>[componentName] =
        [componentName](entt::registry& registry, entt::entity entity, Archive& archive)
    {
        // bool hasComponent;
        //  archive(cereal::make_nvp("hasComponent_" + componentName, hasComponent));
        // make_optional_nvp(archive, ("hasComponent_" + componentName).c_str(), hasComponent);
        // if (hasComponent)
        //{
        if (componentName == "Cell")
        {
            bee::Log::Info("Deserializing Cell");
        }
        Component comp;
        // archive(cereal::make_nvp(componentName.c_str(), comp));
        make_optional_nvp(archive, componentName.c_str(), comp);
        registry.emplace<Component>(entity, comp);
        //}
    };
}

void saveRegistry(entt::registry& registry, const fs::path& filename);
void LoadRegistry(entt::registry& registry, const fs::path& filename);

template <class Archive>
void serializeEntity(Archive& archive, entt::registry& registry, entt::entity entity)
{
    // archive(cereal::make_nvp("entityID", entity));
    make_optional_nvp(archive, "entityID", entity);
    for (const auto& [name, serializeFunc] : serialize_functions<Archive>)
    {
        try
        {
            serializeFunc(registry, entity, archive);
        }
        catch (const std::exception& e)
        {
            bee::Log::Error("Failed to serialize component {}: {}", name, e.what());
        }
    }
}

template <class Archive>
std::string serializeEntityString(entt::registry& registry, entt::entity entity)
{
    std::ostringstream os;
    {
        // yes this scope is necessary, this is because if I get the string before the archive goes out of scope, the archive
        // will not finish closing the json object
        Archive archive(os);
        serializeEntity(archive, registry, entity);
    }
    return os.str();
}

template <class Archive>
static void ChangeComponentBasedOnJsonString(const std::string& jsonString, entt::registry& registry, entt::entity entity)
{
    // convert json string to json object
    rapidjson::Document document;
    document.Parse(jsonString.c_str());
    if (document.HasParseError())
    {
        bee::Log::Error("Failed to parse json string: {}", jsonString);
        return;
    }

    std::string componentName;
    if (document.IsObject() && document.MemberCount() > 0)
    {
        componentName = document.MemberBegin()->name.GetString();
    }
    else
    {
        bee::Log::Error("Invalid JSON structure or empty document: {}", jsonString);
        return;
    }

    std::istringstream is(jsonString);
    Archive archive(is);
    for (const auto& [name, deserializeFunc] : deserialize_functions<Archive>)
    {
        if (name != componentName) continue;
        try
        {
            deserializeFunc(registry, entity, archive);
        }
        catch (const std::exception& e)
        {
            bee::Log::Error("Failed to deserialize component {}: {}", name, e.what());
        }
    }
}

template <class Archive>
void deserializeEntity(Archive& archive,
                       entt::registry& registry,
                       std::unordered_map<entt::entity, entt::entity>& entity_mapping)
{
    // archive(cereal::make_nvp("entityID", entity));
    entt::entity old_entity;
    make_optional_nvp(archive, "entityID", old_entity);
    entt::entity new_entity = registry.create(old_entity);

    entity_mapping[old_entity] = new_entity;

    for (const auto& [name, deserializeFunc] : deserialize_functions<Archive>)
    {
        try
        {
            bool hasComponent = false;
            std::string component = std::string("hasComponent_") + name;
            const char* expected = component.c_str();
            //const char* actual = archive.getNodeName();
            /*archive(cereal::make_nvp("hasComponent_" + name, hasComponent));*/
            make_optional_nvp(archive, expected, hasComponent);
            if (!hasComponent)
            {
                continue;
                //if (std::string(expected) == std::string(actual))
                //{
                //    continue;
                //}

                //// mark the component as read in the archive
                //bool temp;
                //make_optional_nvp(archive, archive.getNodeName(), temp);
            }
            deserializeFunc(registry, new_entity, archive);
        }
        catch (const std::exception& e)
        {
            bee::Log::Error("Failed to deserialize component {}: {}", name, e.what());
        }
    }

    if (!registry.all_of<Saveable>(new_entity))
    {
        registry.emplace<Saveable>(new_entity);
    }
}

template <class Archive>
void deserializeEntityString(const std::string& data,
                             entt::registry& registry,
                             std::unordered_map<entt::entity, entt::entity>& entity_mapping)
{
    std::stringstream is(data);
    Archive archive(is);
    deserializeEntity(archive, registry, entity_mapping);
}

template <class Archive>
void serialize(Archive& archive, entt::registry& registry)
{
    auto& storage = registry.storage<bee::Saveable>();

    size_t entity_count = storage.size();
    // archive(cereal::make_nvp("entityCount", entity_count));
    make_optional_nvp(archive, "entityCount", entity_count);

    for (auto&& [entity] : storage.each())
    {
        serializeEntity(archive, registry, entity);
    }
}

template <class Archive>
void deserialize(Archive& archive, entt::registry& registry)
{
    size_t entity_count;
    try
    {
        // archive(cereal::make_nvp("entityCount", entity_count));
        make_optional_nvp(archive, "entityCount", entity_count);
    }
    catch (const std::exception& e)
    {
        bee::Log::Error("Failed to deserialize entity count: {}", e.what());
        return;
    }

    std::unordered_map<entt::entity, entt::entity> entity_mapping;
    for (size_t i = 0; i < entity_count; ++i)
    {
        try
        {
            deserializeEntity(archive, registry, entity_mapping);
        }
        catch (const std::exception& e)
        {
            bee::Log::Error("Failed to deserialize entity: {}", e.what());
        }
    }

    ecs::UpdateEntityMapping(entity_mapping, registry);

    // loop through every grid component
    auto grid_view = registry.view<bee::Grid, bee::Transform>();
    for (auto entity : grid_view)
    {
        bee::GridManager::InitializeGrid(entity, registry, false);
    }

    // loop through every grid and update the entities old entity to the new entity
    auto view = registry.view<bee::Cell>();
    for (auto entity : view)
    {
        auto& cell = registry.get<bee::Cell>(entity);
        if (entity_mapping.find(cell.entity) != entity_mapping.end())
        {
            cell.entity = entity_mapping[cell.entity];
        }

        if (entity_mapping.find(cell.gridParent) != entity_mapping.end())
        {
            cell.gridParent = entity_mapping[cell.gridParent];
        }
        else
        {
            continue;
        }

        Grid& grid = registry.get<Grid>(cell.gridParent);
        grid.cells[cell.gridPosition.x][cell.gridPosition.y] = entity;
    }

    // loop through every grid component
    for (auto entity : grid_view)
    {
        bee::GridManager::ResizeGrid(entity, registry);
    }
}

void saveEntity(entt::registry& registry, entt::entity entity, const fs::path& filename);
entt::entity loadEntity(entt::registry& registry, const fs::path& filename);

template <class Archive>
void serialize(Archive& archive, entt::registry& registry, entt::entity entity)
{
    try
    {
        serializeEntity(archive, registry, entity);
    }
    catch (const std::exception& e)
    {
        bee::Log::Error("Failed to serialize entity: {}", e.what());
    }
}

template <class Archive>
void deserialize(Archive& archive, entt::registry& registry, entt::entity& entity)
{
    std::unordered_map<entt::entity, entt::entity> entity_mapping;
    try
    {
        deserializeEntity(archive, registry, entity_mapping);
    }
    catch (const std::exception& e)
    {
        bee::Log::Error("Failed to deserialize entity: {}", e.what());
    }

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

    entity = entity_mapping[entity];
}

}  // namespace bee