#pragma once

#include <glm/glm.hpp>
#include <entt/entt.hpp>

namespace bee
{
namespace raycasting
{

struct HitInfo
{
    bool hit;
    float distance;
    glm::vec3 position;
    glm::vec3 normal;
};

struct Ray
{
    glm::vec3 origin;
    glm::vec3 direction;

    Ray() : origin(glm::vec3(0.0f)), direction(glm::vec3(0.0f)) {}
    Ray(const glm::vec3& origin, const glm::vec3& direction) : origin(origin), direction(direction) {}

    glm::vec3 GetPoint(float distance) const { return origin + direction * distance; }

    HitInfo IntersectTransform(glm::mat4 transform, glm::vec3 min, glm::vec3 max) const;
};

glm::vec3 GetEntityScaler(const entt::registry& registry, entt::entity entity);
glm::vec3 GetEntityOffset(const entt::registry& registry, entt::entity entity);
HitInfo IntersectRayWithEntity(const Ray& ray, const glm::mat4& transform, const glm::vec3& scaler = glm::vec3(1.0f));

};  // namespace raycasting
}  // namespace bee