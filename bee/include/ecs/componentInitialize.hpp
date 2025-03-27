#pragma once
#include "common.hpp"
#include "ecs/components.hpp"

namespace bee
{
namespace ecs
{

entt::entity CreateEmpty();
entt::entity CreateDefault(const std::string& name = ICON_FA_SHAPES SPACE_FA "Empty");
entt::entity CreateMesh();
entt::entity CreateDirectionalLight();
entt::entity CreatePointLight();
entt::entity CreateEmitter();
entt::entity CreateSceneData();
entt::entity CreateCamera();
entt::entity CreateGrid();
entt::entity CreateCanvas();
entt::entity CreateCanvasElement();


template <typename... ComponentTypes, typename... Excluded>
auto GetView(entt::registry& registry, entt::type_list<Excluded...> = {})
{
    return registry.view<ComponentTypes...>(entt::exclude<bee::Disabled, Excluded...>);
}

}  // namespace ecs
}  // namespace bee


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/