#pragma once
#include "bee.hpp"

namespace std
{
template <>
struct hash<glm::ivec3>
{
    std::size_t operator()(const glm::ivec3& v) const noexcept
    {
        std::size_t h1 = std::hash<int>{}(v.x);
        std::size_t h2 = std::hash<int>{}(v.y);
        std::size_t h3 = std::hash<int>{}(v.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);  // Combine hashes
    }
};
}  // namespace std

class Gameplay : public bee::Layer
{
public:
    Gameplay() : Layer("Gameplay") {}

    void OnAttach() override;
    void OnDetach() override;
    void OnRender() override;
    void OnEngineInit() override;
    void OnImGuiRender() override;
    void OnUpdate(float deltaTime) override;
    void OnFixedUpdate(float deltaTime) override;

    void OnEvent(bee::Event& e) override;

private:
    entt::entity m_cubePrefab = entt::null;

    std::unordered_map<glm::int3, entt::entity> m_maze;

    glm::int3 m_position;
    std::stack<glm::int3> m_path;
    std::stack<glm::int3> m_longestPath;

    glm::int3 m_size = {10, 10, 10};

    bool m_autoStep = true;
    int m_stepsPerFrame = 1;

private:
    bool Step();
    bool IsOccupied(const glm::int3& position) const;

    void ShowLongestPath();
};


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/