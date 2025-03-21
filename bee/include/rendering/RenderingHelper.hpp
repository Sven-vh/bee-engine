#pragma once
#include "common.hpp"
#include "ecs/components.hpp"

namespace bee
{
namespace helper
{
glm::mat4 LookToPosition(const glm::mat4& original,
                         const glm::vec3& cameraPosition,
                         const bool addRotation,
                         const glm::vec3& customScale = glm::vec3(-1.0f));

glm::mat4 LookToPosition(const glm::vec3& original, const glm::vec3& lookAtPos);
}  // namespace helper
}  // namespace bee