#pragma once
#include "Bee.hpp"
#include "common.hpp"

//written by ChatGPT
struct EntityPairHash
{
    std::size_t operator()(const std::pair<entt::entity, entt::entity>& pair) const
    {
        return std::hash<int>()((int)pair.first) ^ std::hash<int>()((int)pair.second);
    }
};

class Physics : public bee::Layer
{
public:
    Physics() : Layer("Physics") {}
    ~Physics() = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnRender() override;
    void OnEngineInit() override;
    void OnImGuiRender() override;
    void OnUpdate(float deltaTime) override;
    void OnFixedUpdate(float deltaTime) override;

    struct CollisionInfo
    {
        glm::vec3 worldPosition;
        glm::vec3 normal;
        float impactVelocity;
    };

    bool HasCollided(entt::entity entityA, entt::entity entityB, CollisionInfo& info) const;

private:
    float m_gravity = -9.81f;
    float m_reflectionCoefficient = 0.85f;

    // Define a struct for the Oriented Bounding Box (OBB), written by ChatGPT
    struct OBB
    {
        glm::vec3 center;
        glm::vec3 axes[3];    // Normalized local axes (rotation)
        glm::vec3 halfSizes;  // Half-sizes along each local axis (including scaling)

        // copy constructor
        OBB(const OBB& other)
        {
            center = other.center;
            axes[0] = other.axes[0];
            axes[1] = other.axes[1];
            axes[2] = other.axes[2];
            halfSizes = other.halfSizes;
        }

        // default constructor
        OBB() = default;

        // assignment operator
        OBB& operator=(const OBB& other)
        {
            center = other.center;
            axes[0] = other.axes[0];
            axes[1] = other.axes[1];
            axes[2] = other.axes[2];
            halfSizes = other.halfSizes;
            return *this;
        }
    };

    std::unordered_map<std::pair<entt::entity, entt::entity>, CollisionInfo, EntityPairHash> m_collisions;
    std::unordered_map<entt::entity, OBB> entityOBBs;

    void UpdatePositions(float deltaTime);
    void CheckCollisions();
    bool LineSegmentOBBIntersection(const glm::vec3& start,
                                    const glm::vec3& end,
                                    const OBB& obb,
                                    float& t,
                                    glm::vec3& intersectionPoint,
                                    glm::vec3& normal);
};


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/