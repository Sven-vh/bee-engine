#pragma once
#include "common.hpp"

namespace bee
{
class GridManager
{
public:
    static void InitializeGrid(entt::entity gridEntity, entt::registry& registry, bool fillGrid = true);
    static entt::entity CreateNewCell(entt::registry& registry, entt::entity grid, int x, int z);
    static void DrawCells(entt::registry& registry);
    static void ResizeGrid(entt::entity gridEntity, entt::registry& registry);
};
}  // namespace bee


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/