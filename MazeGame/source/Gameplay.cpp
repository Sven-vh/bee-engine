#include "gameplay.hpp"
#include <algorithm>
#include <random>

constexpr glm::int3 directions[] = {
    {0, 0, 1},
    {0, 0, -1},
    {0, 1, 0},
    {0, -1, 0},
    {1, 0, 0},
    {-1, 0, 0},
};

void Gameplay::OnAttach()
{
    //auto& registry = bee::Engine.Registry();
    //auto path = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::SaveFiles, "UITest.scene");
    //bee::LoadRegistry(registry, path);

    //return;
    auto& registry = bee::Engine.Registry();
    auto path = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::SaveFiles, "Maze.scene");
    bee::LoadRegistry(registry, path);

    auto cube = bee::ecs::FindEntitiesByName(bee::Engine.Registry(), "Cube");
    GAME_EXCEPTION_IF(cube.empty(), "No cube found in Scene");
    m_cubePrefab = cube.front();

    m_position = {0, 0, 0};
    m_maze[m_position] = m_cubePrefab;
    bee::Transform& transform = bee::Engine.Registry().get<bee::Transform>(m_cubePrefab);
    transform.SetPosition(glm::vec3(m_position * 2));
}

void Gameplay::OnDetach()
{
    m_maze.clear();
    m_path = std::stack<glm::int3>();
}

void Gameplay::OnRender() {}

void Gameplay::OnEngineInit() {}

void Gameplay::OnImGuiRender()
{
    ImGui::Begin("Gameplay");
    if (ImGui::Button("Step"))
    {
        Step();
    }

    ImGui::Checkbox("Auto Step", &m_autoStep);
    ImGui::DragInt("Steps per Frame", &m_stepsPerFrame, 1.0f, 1, 100);
    ImGui::DragInt3("Size", &m_size.x, 1.0f, 1, 100);
    ImGui::Text("Use the ditor camera to fly around");
    //if (ImGui::Button("Show Longest Path"))
    //{
    //    ShowLongestPath();
    //}

    ImGui::End();
}

void Gameplay::OnUpdate(float) {}

void Gameplay::OnFixedUpdate(float)
{
    if (m_autoStep)
    {
        for (int i = 0; i < m_stepsPerFrame; i++)
        {
            m_autoStep = Step();
            if (!m_autoStep) break;
        }
    }
}

void Gameplay::OnEvent(bee::Event&) {}

bool Gameplay::Step()
{
    while (true)
    {
        std::array<int, 6> indices = {0, 1, 2, 3, 4, 5};
        std::shuffle(indices.begin(), indices.end(), std::mt19937(std::random_device()()));

        int i = 0;
        glm::int3 direction;
        glm::int3 next;

        do
        {
            direction = directions[indices[i]];
            next = m_position + direction;
            i++;
        } while (IsOccupied(next) && i < indices.size());

        if (i == indices.size())
        {
            if (m_path.empty())
            {
                return false;  // No more steps to take
            }

            // Move back to the previous position
            m_position = m_path.top();
            m_path.pop();
        }
        else
        {
            // Move to the next position
            m_path.push(m_position);
            m_position = next;
            entt::entity newEntity = bee::ecs::DuplicateEntity(bee::Engine.Registry(), m_cubePrefab);
            m_maze[m_position] = newEntity;
            bee::Transform& transform = bee::Engine.Registry().get<bee::Transform>(newEntity);
            transform.SetPosition(glm::vec3(m_position * 2));

            entt::entity averageEntity = bee::ecs::DuplicateEntity(bee::Engine.Registry(), m_cubePrefab);
            /*m_maze[m_position] = averageEntity;*/
            bee::Transform& averageTransform = bee::Engine.Registry().get<bee::Transform>(averageEntity);
            averageTransform.SetPosition(glm::vec3(m_position * 2 - direction));

            if (m_path.size() > m_longestPath.size())
            {
                m_longestPath = m_path;
            }

            return true;
        }
    }
}

bool Gameplay::IsOccupied(const glm::int3& position) const
{
    if (position.x < 0 || position.x >= m_size.x || position.y < 0 || position.y >= m_size.y || position.z < 0 ||
        position.z >= m_size.z)
        return true;
    return m_maze.find(position) != m_maze.end();
}

void Gameplay::ShowLongestPath()
{
    while (!m_longestPath.empty())
    {
        glm::int3 position = m_longestPath.top();
        m_longestPath.pop();
        entt::entity entity = m_maze[position];
        bee::Renderable& renderable = bee::Engine.Registry().get<bee::Renderable>(entity);
        renderable.tint = glm::vec4(0, 1, 0, 1);
    }
}
