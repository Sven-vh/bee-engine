#include "tools/shapes.hpp"
#include "core.hpp"

#define SPHERE_SEGMENTS 16
#define ARROW_SEGMENTS 16
#define ARROW_LENGTH 1.0f
#define SPHERE_RADIUS 1.0f
#define DEFAULT_COLOR glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)
#define CONE_SEGMENTS 16
#define CONE_HEIGHT 1.0f

// All of these functions were written by ChatGPT
bool bee::DebugRenderer::DrawSphere(glm::vec3 position, float radius, glm::vec4 color)
{
    for (int i = 0; i <= SPHERE_SEGMENTS; ++i)
    {
        float theta1 = (float(i) / float(SPHERE_SEGMENTS)) * glm::pi<float>();
        float theta2 = (float(i + 1) / float(SPHERE_SEGMENTS)) * glm::pi<float>();

        for (int j = 0; j <= SPHERE_SEGMENTS; ++j)
        {
            float phi1 = (float(j) / float(SPHERE_SEGMENTS)) * glm::two_pi<float>();
            float phi2 = (float(j + 1) / float(SPHERE_SEGMENTS)) * glm::two_pi<float>();

            glm::vec3 p1 = position + glm::vec3(radius * glm::sin(theta1) * glm::cos(phi1),
                                                radius * glm::cos(theta1),
                                                radius * glm::sin(theta1) * glm::sin(phi1));
            glm::vec3 p2 = position + glm::vec3(radius * glm::sin(theta2) * glm::cos(phi1),
                                                radius * glm::cos(theta2),
                                                radius * glm::sin(theta2) * glm::sin(phi1));
            glm::vec3 p3 = position + glm::vec3(radius * glm::sin(theta2) * glm::cos(phi2),
                                                radius * glm::cos(theta2),
                                                radius * glm::sin(theta2) * glm::sin(phi2));
            glm::vec3 p4 = position + glm::vec3(radius * glm::sin(theta1) * glm::cos(phi2),
                                                radius * glm::cos(theta1),
                                                radius * glm::sin(theta1) * glm::sin(phi2));

            xsr::render_debug_line(glm::value_ptr(p1), glm::value_ptr(p2), glm::value_ptr(color));
            xsr::render_debug_line(glm::value_ptr(p2), glm::value_ptr(p3), glm::value_ptr(color));
            xsr::render_debug_line(glm::value_ptr(p3), glm::value_ptr(p4), glm::value_ptr(color));
            xsr::render_debug_line(glm::value_ptr(p4), glm::value_ptr(p1), glm::value_ptr(color));
        }
    }
    return true;
}

bool bee::DebugRenderer::DrawArrow(glm::vec3 position, glm::vec3 direction, float length, glm::vec4 color)
{
    glm::vec3 endPoint = position + glm::normalize(direction) * length;
    xsr::render_debug_line(glm::value_ptr(position), glm::value_ptr(endPoint), glm::value_ptr(color));

    // Draw arrowhead
    glm::vec3 orthogonal1 = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f))) * (length * 0.1f);
    glm::vec3 orthogonal2 = glm::normalize(glm::cross(direction, glm::vec3(1.0f, 0.0f, 0.0f))) * (length * 0.1f);
    glm::vec3 arrowHead1 = endPoint - glm::normalize(direction) * (length * 0.2f) + orthogonal1;
    glm::vec3 arrowHead2 = endPoint - glm::normalize(direction) * (length * 0.2f) - orthogonal1;
    glm::vec3 arrowHead3 = endPoint - glm::normalize(direction) * (length * 0.2f) + orthogonal2;
    glm::vec3 arrowHead4 = endPoint - glm::normalize(direction) * (length * 0.2f) - orthogonal2;

    xsr::render_debug_line(glm::value_ptr(endPoint), glm::value_ptr(arrowHead1), glm::value_ptr(color));
    xsr::render_debug_line(glm::value_ptr(endPoint), glm::value_ptr(arrowHead2), glm::value_ptr(color));
    xsr::render_debug_line(glm::value_ptr(endPoint), glm::value_ptr(arrowHead3), glm::value_ptr(color));
    xsr::render_debug_line(glm::value_ptr(endPoint), glm::value_ptr(arrowHead4), glm::value_ptr(color));

    return true;
}

bool bee::DebugRenderer::DrawBox(glm::vec3 position, glm::vec3 size, glm::vec4 color)
{
    // Calculate the 8 corners of the box based on position and size
    glm::vec3 halfSize = size * 0.5f;
    glm::vec3 p1 = position + glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z);
    glm::vec3 p2 = position + glm::vec3(halfSize.x, -halfSize.y, -halfSize.z);
    glm::vec3 p3 = position + glm::vec3(halfSize.x, halfSize.y, -halfSize.z);
    glm::vec3 p4 = position + glm::vec3(-halfSize.x, halfSize.y, -halfSize.z);
    glm::vec3 p5 = position + glm::vec3(-halfSize.x, -halfSize.y, halfSize.z);
    glm::vec3 p6 = position + glm::vec3(halfSize.x, -halfSize.y, halfSize.z);
    glm::vec3 p7 = position + glm::vec3(halfSize.x, halfSize.y, halfSize.z);
    glm::vec3 p8 = position + glm::vec3(-halfSize.x, halfSize.y, halfSize.z);

    // Draw the 12 edges of the box
    xsr::render_debug_line(glm::value_ptr(p1), glm::value_ptr(p2), glm::value_ptr(color));
    xsr::render_debug_line(glm::value_ptr(p2), glm::value_ptr(p3), glm::value_ptr(color));
    xsr::render_debug_line(glm::value_ptr(p3), glm::value_ptr(p4), glm::value_ptr(color));
    xsr::render_debug_line(glm::value_ptr(p4), glm::value_ptr(p1), glm::value_ptr(color));

    xsr::render_debug_line(glm::value_ptr(p5), glm::value_ptr(p6), glm::value_ptr(color));
    xsr::render_debug_line(glm::value_ptr(p6), glm::value_ptr(p7), glm::value_ptr(color));
    xsr::render_debug_line(glm::value_ptr(p7), glm::value_ptr(p8), glm::value_ptr(color));
    xsr::render_debug_line(glm::value_ptr(p8), glm::value_ptr(p5), glm::value_ptr(color));

    xsr::render_debug_line(glm::value_ptr(p1), glm::value_ptr(p5), glm::value_ptr(color));
    xsr::render_debug_line(glm::value_ptr(p2), glm::value_ptr(p6), glm::value_ptr(color));
    xsr::render_debug_line(glm::value_ptr(p3), glm::value_ptr(p7), glm::value_ptr(color));
    xsr::render_debug_line(glm::value_ptr(p4), glm::value_ptr(p8), glm::value_ptr(color));

    return true;
}

bool bee::DebugRenderer::DrawTransformedBox(const glm::mat4& transform, glm::vec3 size, glm::vec4 color)
{
    glm::vec3 halfSize = size * 0.5f;
    glm::vec3 localCorners[8] = {glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z),
                                 glm::vec3(halfSize.x, -halfSize.y, -halfSize.z),
                                 glm::vec3(halfSize.x, halfSize.y, -halfSize.z),
                                 glm::vec3(-halfSize.x, halfSize.y, -halfSize.z),
                                 glm::vec3(-halfSize.x, -halfSize.y, halfSize.z),
                                 glm::vec3(halfSize.x, -halfSize.y, halfSize.z),
                                 glm::vec3(halfSize.x, halfSize.y, halfSize.z),
                                 glm::vec3(-halfSize.x, halfSize.y, halfSize.z)};

    glm::vec3 worldCorners[8];
    for (int i = 0; i < 8; ++i)
    {
        worldCorners[i] = glm::vec3(transform * glm::vec4(localCorners[i], 1.0f));
    }

    for (int i = 0; i < 4; ++i)
    {
        xsr::render_debug_line(glm::value_ptr(worldCorners[i]),
                               glm::value_ptr(worldCorners[(i + 1) % 4]),
                               glm::value_ptr(color));
        xsr::render_debug_line(glm::value_ptr(worldCorners[i + 4]),
                               glm::value_ptr(worldCorners[(i + 1) % 4 + 4]),
                               glm::value_ptr(color));
        xsr::render_debug_line(glm::value_ptr(worldCorners[i]), glm::value_ptr(worldCorners[i + 4]), glm::value_ptr(color));
    }

    return true;
}

bool bee::DebugRenderer::DrawCone(glm::vec3 position, glm::vec3 direction, float angle, glm::vec4 color)
{
    glm::vec3 normalizedDir = glm::normalize(direction);

    // Check if angle is greater than 90 degrees
    if (angle > 90.0f)
    {
        // Adjust the direction to point in the opposite direction
        normalizedDir = -normalizedDir;
        angle = 180.0f - angle;  // Adjust the angle to be within 0 to 90 range
    }

    glm::vec3 baseCenter = position + normalizedDir * CONE_HEIGHT;

    glm::vec3 orthogonalVec;
    if (glm::abs(normalizedDir.y - 1.0f) < 0.001f)
    {
        orthogonalVec = glm::vec3(1.0f, 0.0f, 0.0f);  // Handle case when direction is (0,1,0)
    }
    else if (glm::abs(normalizedDir.y + 1.0f) < 0.001f)
    {
        orthogonalVec = glm::vec3(-1.0f, 0.0f, 0.0f);  // Handle case when direction is (0,-1,0)
    }
    else
    {
        orthogonalVec = glm::normalize(glm::cross(normalizedDir, glm::vec3(0.0f, 1.0f, 0.0f)));
        if (glm::length(orthogonalVec) < 0.001f)
        {
            orthogonalVec = glm::normalize(glm::cross(normalizedDir, glm::vec3(1.0f, 0.0f, 0.0f)));
        }
    }

    // Calculate the radius with the adjusted angle
    float radius = CONE_HEIGHT * glm::tan(glm::radians(angle));

    for (int i = 0; i < CONE_SEGMENTS; ++i)
    {
        float theta1 = (float(i) / float(CONE_SEGMENTS)) * glm::two_pi<float>();
        float theta2 = (float(i + 1) / float(CONE_SEGMENTS)) * glm::two_pi<float>();

        glm::quat rotation1 = glm::angleAxis(theta1, normalizedDir);
        glm::quat rotation2 = glm::angleAxis(theta2, normalizedDir);

        glm::vec3 offset1 = rotation1 * (orthogonalVec * radius);
        glm::vec3 offset2 = rotation2 * (orthogonalVec * radius);

        glm::vec3 point1 = baseCenter + offset1;
        glm::vec3 point2 = baseCenter + offset2;

        // Draw lines from apex to base points
        xsr::render_debug_line(glm::value_ptr(position), glm::value_ptr(point1), glm::value_ptr(color));

        // Draw lines connecting base points
        xsr::render_debug_line(glm::value_ptr(point1), glm::value_ptr(point2), glm::value_ptr(color));
    }

    return true;
}

// made by ChatGPT
bool bee::DebugRenderer::DrawFrustum(glm::mat4& viewMatrix, glm::mat4& projectionMatrix, glm::vec4 color, bool isOrthographic)
{
    glm::mat4 invVP = glm::inverse(projectionMatrix * viewMatrix);

    std::array<glm::vec4, 8> frustumCorners;

    if (isOrthographic)
    {
        // For orthographic projections, the corners should be evenly spaced along the Z-axis
        frustumCorners = {
            glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),  // Near bottom-left
            glm::vec4(1.0f, -1.0f, -1.0f, 1.0f),   // Near bottom-right
            glm::vec4(1.0f, 1.0f, -1.0f, 1.0f),    // Near top-right
            glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),   // Near top-left
            glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),   // Far bottom-left
            glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),    // Far bottom-right
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),     // Far top-right
            glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f)     // Far top-left
        };
    }
    else
    {
        // For perspective projections
        frustumCorners = {
            glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),  // Near bottom-left
            glm::vec4(1.0f, -1.0f, 0.0f, 1.0f),   // Near bottom-right
            glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),    // Near top-right
            glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f),   // Near top-left
            glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),  // Far bottom-left
            glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),   // Far bottom-right
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),    // Far top-right
            glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f)    // Far top-left
        };
    }

    for (auto& corner : frustumCorners)
    {
        corner = invVP * corner;
        corner /= corner.w;
    }

    for (int i = 0; i < 4; ++i)
    {
        xsr::render_debug_line(glm::value_ptr(glm::vec3(frustumCorners[i])),
                               glm::value_ptr(glm::vec3(frustumCorners[(i + 1) % 4])),
                               glm::value_ptr(color));  // Near plane
        xsr::render_debug_line(glm::value_ptr(glm::vec3(frustumCorners[i + 4])),
                               glm::value_ptr(glm::vec3(frustumCorners[((i + 1) % 4) + 4])),
                               glm::value_ptr(color));  // Far plane
        xsr::render_debug_line(glm::value_ptr(glm::vec3(frustumCorners[i])),
                               glm::value_ptr(glm::vec3(frustumCorners[i + 4])),
                               glm::value_ptr(color));  // Connecting lines
    }

    return true;
}

bool bee::DebugRenderer::DrawLine(glm::vec3 start, glm::vec3 end, glm::vec4 color)
{
    xsr::render_debug_line(glm::value_ptr(start), glm::value_ptr(end), glm::value_ptr(color));
    return true;
}