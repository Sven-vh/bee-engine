#pragma once
#include "common.hpp"

namespace bee
{
class DebugRenderer
{
public:
    static bool DrawSphere(glm::vec3 position, float radius, glm::vec4 color);
    static bool DrawArrow(glm::vec3 position, glm::vec3 direction, float length, glm::vec4 color);
    static bool DrawBox(glm::vec3 position, glm::vec3 size, glm::vec4 color);
    static bool DrawTransformedBox(const glm::mat4& transform, glm::vec3 size, glm::vec4 color);
    static bool DrawCone(glm::vec3 position, glm::vec3 direction, float angle, glm::vec4 color);
    static bool DrawFrustum(glm::mat4& viewMatrix, glm::mat4& projectionMatrix, glm::vec4 color, bool isOrthographic);
    static bool DrawLine(glm::vec3 start, glm::vec3 end, glm::vec4 color);
};  // namespace shapes
}  // namespace bee