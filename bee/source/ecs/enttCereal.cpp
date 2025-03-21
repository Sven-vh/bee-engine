#include "ecs/enttCereal.hpp"
#include "core.hpp"
#include "cereal/cereal_optional_nvp.h"

void bee::saveRegistry(entt::registry& registry, const fs::path& filename)
{
    // check if the file and directory exists
    if (!std::filesystem::exists(filename))
    {
        // create the directory
        std::filesystem::create_directories(std::filesystem::path(filename).parent_path());
    }

    std::ofstream os(filename);
    cereal::JSONOutputArchive archive(os);
    serialize(archive, registry);  // Serialize entire registry
}

void bee::LoadRegistry(entt::registry& registry, const fs::path& filename)
{
    std::ifstream is(filename);

    // check if it exists
    if (!std::filesystem::exists(filename))
    {
        bee::Log::Error("File does not exist: {}", filename.string());
        return;
    }
    // check if it is open
    if (!is.is_open())
    {
        bee::Log::Error("Failed to open file: {}", filename.string());
        return;
    }

    cereal::JSONInputArchive archive(is);
    deserialize(archive, registry);  // Deserialize entire registry

    // loop through every gltfScene component
    auto view = registry.view<bee::GltfScene>();
    for (auto entity : view)
    {
        bee::resource::LoadGLTFFromEntity(entity, bee::Engine.Registry());
    }
}

void bee::saveEntity(entt::registry& registry, entt::entity entity, const fs::path& filename)
{
    auto children = bee::ecs::GetAllChildren(registry, entity);
    std::ofstream os(filename);
    cereal::JSONOutputArchive archive(os);
    make_optional_nvp(archive, "entityCount", children.size() + 1);
    serialize(archive, registry, entity);  // Serialize single entity
    for (auto child : children)
    {
        serialize(archive, registry, child);
    }
}

entt::entity bee::loadEntity(entt::registry& registry, const fs::path& filename)
{
    std::ifstream is(filename);

    if (!is.is_open())
    {
        bee::Log::Error("Failed to open file: {}", filename.string());
        return entt::null;
    }


    cereal::JSONInputArchive archive(is);
    deserialize(archive, registry);  // Deserialize single entity

        // loop through every gltfScene component
    auto view = registry.view<bee::GltfScene>();
    for (auto entity : view)
    {
        bee::resource::LoadGLTFFromEntity(entity, bee::Engine.Registry());
    }

    entt::entity newEntity = entt::null;

    return newEntity;
}
