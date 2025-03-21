#include "tools/raycasting.hpp"
#include "core.hpp"

bee::raycasting::HitInfo bee::raycasting::Ray::IntersectTransform(glm::mat4 transform, glm::vec3 min, glm::vec3 max) const
{
    glm::mat4 invTransform = glm::inverse(transform);

    glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(origin, 1.0f));
    glm::vec3 localDirection = glm::normalize(glm::vec3(invTransform * glm::vec4(direction, 0.0f)));

    glm::vec3 invLocalDir = 1.0f / localDirection;

    float tmin = 0;
    float tmax = std::numeric_limits<float>::infinity();

    glm::vec3 hitNormal(0.0f);
    glm::vec3 normalSigns[] = {glm::vec3(-1, 0, 0),
                               glm::vec3(1, 0, 0),
                               glm::vec3(0, -1, 0),
                               glm::vec3(0, 1, 0),
                               glm::vec3(0, 0, -1),
                               glm::vec3(0, 0, 1)};

    for (int i = 0; i < 3; i++)
    {
        float t1 = (min[i] - localOrigin[i]) * invLocalDir[i];
        float t2 = (max[i] - localOrigin[i]) * invLocalDir[i];

        float tmin_i = std::min(t1, t2);
        float tmax_i = std::max(t1, t2);

        // Determine which face the ray is intersecting to calculate normal
        if (t1 < t2 && tmin_i > tmin)
            hitNormal = normalSigns[i * 2];
        else if (t2 < t1 && tmin_i > tmin)
            hitNormal = normalSigns[i * 2 + 1];

        tmin = std::max(tmin, tmin_i);
        tmax = std::min(tmax, tmax_i);

        if (tmax < tmin)
        {
            return {false, 0.0f, glm::vec3(0.0f), glm::vec3(0.0f)};  // No intersection
        }
    }

    if (tmin < 0)
    {
        return {false, 0.0f, glm::vec3(0.0f), glm::vec3(0.0f)};  // Behind the ray origin
    }

    glm::vec3 hitPosition = localOrigin + localDirection * tmin;
    hitPosition = glm::vec3(transform * glm::vec4(hitPosition, 1.0f));

    // Transform hit normal back to world space
    hitNormal = glm::normalize(glm::vec3(transform * glm::vec4(hitNormal, 0.0f)));

    return {true, tmin, hitPosition, hitNormal};
}

glm::vec3 bee::raycasting::GetEntityScaler(const entt::registry& registry, entt::entity entity)
{
    glm::vec3 scaler(1.0f);
    if (registry.all_of<Renderable>(entity))
    {
        const auto& mesh = registry.get<Renderable>(entity);
        if (mesh.visible)
        {
            scaler = mesh.mesh->GetHandle().meshSize;
        }
    }
    return scaler;
}

glm::vec3 bee::raycasting::GetEntityOffset(const entt::registry& registry, entt::entity entity)
{
    glm::vec3 offset(0.0f);
    if (registry.all_of<Renderable>(entity))
    {
        const auto& mesh = registry.get<Renderable>(entity);
        if (mesh.visible)
        {
            offset = mesh.mesh->GetHandle().meshCenter;
        }
    }
    return offset;
}

bee::raycasting::HitInfo bee::raycasting::IntersectRayWithEntity(const Ray& ray,
                                                                 const glm::mat4& transform,
                                                                 const glm::vec3& scaler)
{
    return ray.IntersectTransform(transform, glm::vec3(-0.5f) * scaler, glm::vec3(0.5f) * scaler);
}