#include "core.hpp"
#include "managers/scene_manager.hpp"

void bee::ecs::LoadScene(const std::string& path) { 
    bee::LoadRegistry(bee::Engine.Registry(), path); 
}

void bee::ecs::UnloadScene()
{
    auto& registry = bee::Engine.Registry();

    auto& storage = registry.storage<entt::entity>();
    for (auto&& [entity] : storage.each())
    {
        if (!registry.any_of<bee::EditorComponent>(entity))
        {
            registry.destroy(entity);
        }
    }
}



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/