#include "managers/grid_manager.hpp"
#include "core.hpp"

// showcase of how the grid struct is implemented
// struct Grid
//{
//    int width = 10;
//    int height = 10;
//    float tileSize = 1.0f;
//    float spacing = 0.1f;
//    bool showGrid = true;
//
//    std::vector<std::vector<entt::entity>> cells;
//};

// struct Cell
//{
//     entt::entity entity = entt::null;
//
// };

static inline glm::vec3 GetCellPosition(const bee::Grid& grid, int x, int z)
{
    return glm::vec3(x * grid.tileSize + grid.spacing * x, 0.0f, z * grid.tileSize + grid.spacing * z);
}

void bee::GridManager::InitializeGrid(entt::entity gridEntity, entt::registry& registry, bool fillGrid)
{
    // if (registry.all_of<Grid>(entity)) return;

    Grid& grid = registry.get<Grid>(gridEntity);

    // Clear the grid
    for (int x = 0; x < (int)grid.cells.size(); x++)
    {
        for (int z = 0; z < (int)grid.cells[x].size(); z++)
        {
            if (registry.valid(grid.cells[x][z]))
            {
                registry.destroy(grid.cells[x][z]);
            }
        }
    }

    grid.cells.clear();
    grid.cells.resize(grid.width);

    // Create the grid
    for (int x = 0; x < grid.width; x++)
    {
        for (int z = 0; z < grid.height; z++)
        {
            grid.cells[x].push_back(fillGrid ? CreateNewCell(registry, gridEntity, x, z) : entt::null);
        }
    }
}

entt::entity bee::GridManager::CreateNewCell(entt::registry& registry, entt::entity gridEntity, int x, int z)
{
    entt::entity cell = bee::ecs::CreateEmpty();
    registry.emplace<Cell>(cell);
    registry.get<Cell>(cell).gridParent = gridEntity;
    registry.get<Cell>(cell).gridPosition = glm::ivec2(x, z);
    registry.get<Cell>(cell).entity = entt::null;
    registry.emplace<Transform>(cell);
    Grid& gridData = registry.get<Grid>(gridEntity);
    registry.get<Transform>(cell).SetPosition(GetCellPosition(gridData, x, z));
    registry.get<Transform>(cell).SetScale(glm::vec3(1.0f, 0.1f, 1.0f));
    registry.emplace<Raycastable>(cell);
    registry.emplace<Saveable>(cell);
    return cell;
}

void bee::GridManager::DrawCells(entt::registry& registry)
{
    const auto view = registry.view<Grid>();
    for (const auto entity : view)
    {
        Grid& grid = view.get<Grid>(entity);
        if (!grid.showGrid) continue;

        for (int x = 0; x < grid.width; x++)
        {
            for (int z = 0; z < grid.height; z++)
            {
                if (grid.cells[x][z] == entt::null) continue;
                const auto& cell = registry.get<Cell>(grid.cells[x][z]);
                auto& cellTransform = registry.get<Transform>(grid.cells[x][z]);

                glm::vec3 size = glm::vec3(1.0f, 0.0f, 1.0f) * grid.tileSize;

                glm::vec4 color =
                    cell.entity == entt::null ? glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) : glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

                glm::mat4 worldPos = GetWorldModel(entity, registry) * cellTransform.GetModelMatrix();
                bee::DebugRenderer::DrawTransformedBox(worldPos, size, color);
            }
        }
    }
}
void bee::GridManager::ResizeGrid(entt::entity gridEntity, entt::registry& registry)
{
    Grid& grid = registry.get<Grid>(gridEntity);

    // clamp the grid size
    grid.width = std::max(1, grid.width);
    grid.height = std::max(1, grid.height);

    // Resize the rows if needed
    if (grid.width > (int)grid.cells.size())
    {
        grid.cells.resize(grid.width);
    }

    for (int x = 0; x < grid.width; x++)
    {
        // Resize the columns if needed for each row
        if (grid.height > (int)grid.cells[x].size())
        {
            for (int z = (int)grid.cells[x].size(); z < grid.height; z++)
            {
                // Create new cells for additional rows/columns
                entt::entity newCell = CreateNewCell(registry, gridEntity, x, z);
                grid.cells[x].push_back(newCell);
            }
        }
        else if (grid.height < (int)grid.cells[x].size())
        {
            // If shrinking, remove excess entities
            for (int z = grid.height; z < (int)grid.cells[x].size(); z++)
            {
                if (registry.valid(grid.cells[x][z]))
                {
                    Cell& cellData = registry.get<Cell>(grid.cells[x][z]);
                    bee::ecs::DestroyEntity(cellData.entity, registry);
                    registry.destroy(grid.cells[x][z]);
                }
            }
            grid.cells[x].resize(grid.height);  // Resize the column to match the new height
        }
    }

    // If grid width shrunk, remove excess rows
    if (grid.width < (int)grid.cells.size())
    {
        for (int x = grid.width; x < (int)grid.cells.size(); x++)
        {
            for (int z = 0; z < (int)grid.cells[x].size(); z++)
            {
                if (registry.valid(grid.cells[x][z]))
                {
                    Cell& cellData = registry.get<Cell>(grid.cells[x][z]);
                    bee::ecs::DestroyEntity(cellData.entity, registry);
                    registry.destroy(grid.cells[x][z]);
                }
            }
        }
        grid.cells.resize(grid.width);  // Resize the rows to match the new width
    }

    // update all the cell positions
    for (int x = 0; x < grid.width; x++)
    {
        for (int z = 0; z < grid.height; z++)
        {
            glm::vec3 position = GetCellPosition(grid, x, z);
            entt::entity cell = grid.cells[x][z];

            if (cell == entt::null) continue;
            if (!registry.valid(cell)) continue;

            registry.get<Transform>(cell).SetPosition(position);
            Cell& cellData = registry.get<Cell>(cell);

            if (cellData.entity == entt::null) continue;

            registry.get<Transform>(cellData.entity).SetPosition(position);
        }
    }
}



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/