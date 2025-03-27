#include "core.hpp"
#include "ecs/components.hpp"
#include "tools/raycasting.hpp"
#include <glm/gtx/matrix_decompose.hpp>

bee::Transform::Transform(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scl)
    : position(pos), rotation(rot), scale(scl)
{
    m_dirty = true;
}

void bee::Transform::SetPosition(const glm::vec3& pos)
{
    position = pos;
    m_dirty = true;
}

void bee::Transform::SetRotationQuat(const glm::quat& rot)
{
    rotation = rot;
    rotationEuler = glm::degrees(glm::eulerAngles(rotation));
    m_dirty = true;
}

void bee::Transform::SetRotation(const glm::vec3& euler)
{
    rotationEuler = euler;
    rotation = glm::quat(glm::radians(rotationEuler));
    m_dirty = true;
}

void bee::Transform::SetScale(const glm::vec3& scl)
{
    scale = scl;
    m_dirty = true;
}

void bee::Transform::RotateAroundAxis(const glm::vec3& axis, float angleDegrees)
{
    glm::vec3 normalizedAxis = glm::normalize(axis);
    float angleRadians = glm::radians(angleDegrees);

    glm::quat rotationDelta = glm::angleAxis(angleRadians, normalizedAxis);
    rotation = rotationDelta * rotation;

    rotationEuler = glm::degrees(glm::eulerAngles(rotation));
    m_dirty = true;
}

glm::vec3 bee::Transform::GetDirection() const
{
    // check if it's not zero vector
    if (rotationEuler == glm::vec3(0.0f))
    {
        return glm::vec3(0.0f, 0.0f, -1.0f);
    }
    return glm::normalize(glm::vec3(glm::rotate(rotation, glm::vec3(0.0f, 0.0f, -1.0f))));
}

glm::vec3 bee::Transform::GetRight() const { return glm::normalize(glm::cross(GetDirection(), glm::vec3(0.0f, 1.0f, 0.0f))); }

glm::vec3 bee::Transform::GetUp() const { return glm::normalize(glm::cross(GetRight(), GetDirection())); }

const glm::mat4& bee::Transform::GetModelMatrix()
{
    if (m_dirty)
    {
        m_model = glm::translate(glm::mat4(1.0f), position) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
        m_dirty = false;
    }
    return m_model;
}

void bee::Transform::SetModelMatrix(const glm::mat4& modelMatrix)
{
    // Decompose the model matrix into translation, rotation, and scale
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(modelMatrix, scale, rotation, position, skew, perspective);

    // Update the Euler rotation angles from the quaternion rotation
    rotationEuler = glm::degrees(glm::eulerAngles(rotation));

    // Mark the model as dirty to recalculate when needed
    m_dirty = false;

    // Store the model matrix directly
    m_model = modelMatrix;
}

bee::Renderable::Renderable()
{
    if (!mesh.get() || !mesh->is_valid())
    {
        mesh = bee::resource::LoadResource<bee::resource::Mesh>(
            bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_MODEL));
    }

    if (!texture.get() || !texture->IsValid())
    {
        texture = bee::resource::LoadResource<bee::resource::Texture>(
            bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_TEXTURE));
    }
}

bee::Renderable::Renderable(const Ref<bee::resource::Mesh>& _newMesh,
                            const Ref<bee::resource::Texture>& _texture,
                            const glm::vec4& multiplier,
                            const glm::vec4& tint,
                            bool visible,
                            bool billboard,
                            bool receive_shadows)
    : mesh((_newMesh)),
      texture((_texture)),
      multiplier(multiplier),
      tint(tint),
      visible(visible),
      billboard(billboard),
      receiveShadows(receive_shadows)
{
    if (!mesh.get() || !mesh->is_valid())
    {
        mesh = bee::resource::LoadResource<bee::resource::Mesh>(
            bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_MODEL));
    }

    if (!texture.get() || !texture->IsValid())
    {
        texture = bee::resource::LoadResource<bee::resource::Texture>(
            bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_TEXTURE));
    }
}

void bee::HierarchyNode::DestroyChildren(entt::registry& registry)
{
    for (auto child : children)
    {
        // check if child is valid and inside the registry

        if (!registry.valid(child))
        {
            continue;
        }

        // check if child has children
        auto* childNode = registry.try_get<HierarchyNode>(child);
        if (childNode)
        {
            childNode->DestroyChildren(registry);
        }
        registry.destroy(child);
    }
    children.clear();
}

bee::EditorIcon::EditorIcon()
{
    if (!icon.get() || !icon->IsValid())
    {
        icon = bee::resource::LoadResource<bee::resource::Texture>(
            bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_TEXTURE));
    }

    quad = bee::resource::LoadResource<bee::resource::Mesh>(
        bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_QUAD));
}

bee::EditorIcon::EditorIcon(const fs::path& iconPath) : iconPath(iconPath)
{
    icon = bee::resource::LoadResource<bee::resource::Texture>(
        bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, iconPath));

    quad = bee::resource::LoadResource<bee::resource::Mesh>(
        bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_QUAD));
}

bee::SceneData::SceneData()
{
    skyColor.addColor(0.49f, glm::vec4(0.62f, 0.62f, 0.62f, 1.0f));
    skyColor.addColor(0.5f, glm::vec4(0.926f, 0.963f, 1.000f, 1.000f));
    skyColor.addColor(0.53f, glm::vec4(0.325f, 0.544f, 0.897f, 1.000f));
    skyColor.addColor(0.75f, glm::vec4(0.341f, 0.471f, 0.678f, 1.000f));
}

bee::Camera::Camera(float fov, float aspectRatio, float nearClip, float farClip)
    : fov(fov), aspectRatio(aspectRatio), nearClip(nearClip), farClip(farClip)
{
}

glm::mat4 bee::Camera::GetProjectionMatrix() const
{
    if (orthographic)
    {
        float left = -orthoSize * aspectRatio;
        float right = orthoSize * aspectRatio;
        float bottom = -orthoSize;
        float top = orthoSize;
        return glm::ortho(left, right, bottom, top, nearClip, farClip);
    }
    return glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
}

glm::mat4 bee::Camera::GetViewMatrix(glm::vec3 position, glm::quat rotation)
{
    glm::mat4 rotationMatrix = glm::toMat4(rotation);

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * rotationMatrix;

    return glm::inverse(transform);
}

glm::mat4 bee::Camera::GetViewProjectionMatrix(glm::vec3 position, glm::quat rotation) const
{
    return GetProjectionMatrix() * GetViewMatrix(position, rotation);
}

bee::raycasting::Ray bee::Camera::ScreenPointToRay(const glm::vec2& screenPoint,
                                                   const glm::vec3& position,
                                                   const glm::quat& rotation,
                                                   const float screenWidth,
                                                   const float screenHeight) const
{
    // Convert screen point to Normalized Device Coordinates (NDC)
    const glm::vec2 normalizedCoords((screenPoint.x / screenWidth) * 2.0f - 1.0f, 1.0f - (screenPoint.y / screenHeight) * 2.0f);

    if (orthographic)
    {
        // **Orthographic Projection**

        // In OpenGL, NDC z ranges from -1 (near) to 1 (far)
        // To get a point on the near plane, set z to -1
        glm::vec4 clipCoords(normalizedCoords.x, normalizedCoords.y, -1.0f, 1.0f);

        // Compute Inverse View-Projection Matrix
        glm::mat4 invVP = glm::inverse(GetProjectionMatrix() * GetViewMatrix(position, rotation));

        // Unproject to World Space
        glm::vec4 worldPos = invVP * clipCoords;
        worldPos /= worldPos.w;  // Perspective divide (though w should be 1 for orthographic)

        auto origin = glm::vec3(worldPos);

        // Direction is the view direction (e.g., -Z in camera space transformed to world space)
        auto direction = glm::normalize(glm::vec3(rotation * glm::vec3(0.0f, 0.0f, -1.0f)));

        return bee::raycasting::Ray(origin, direction);
    }
    else
    {
        // **Perspective Projection**

        // NDC z for the near plane is -1 in OpenGL
        glm::vec4 clipCoords(normalizedCoords.x, normalizedCoords.y, -1.0f, 1.0f);

        // Compute Inverse Projection Matrix
        const glm::mat4 invProj = glm::inverse(GetProjectionMatrix());

        // Unproject to Eye Space
        glm::vec4 eyeCoords = invProj * clipCoords;
        eyeCoords = glm::vec4(eyeCoords.x, eyeCoords.y, -1.0f, 0.0f);  // Set w to 0 for direction vector

        // Compute Inverse View Matrix
        const glm::mat4 invView = glm::inverse(GetViewMatrix(position, rotation));

        // Transform to World Space
        glm::vec3 rayDirection = glm::vec3(invView * eyeCoords);
        rayDirection = glm::normalize(rayDirection);

        // Ray Origin is the camera position
        return bee::raycasting::Ray(position, rayDirection);
    }
}

// enum BorderType
//{
//     Top,
//     Bottom,
//     Left,
//     Right
// };

glm::vec3 bee::Camera::GetAnchorPosition(Anchor type, const glm::vec3& position, const glm::quat& rotation) const
{
    // Compute the camera's forward vector
    glm::vec3 forward = glm::normalize(glm::vec3(rotation * glm::vec3(0.0f, 0.0f, -1.0f)));

    // Compute the camera's right vector
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

    // Compute the camera's up vector
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    // Compute the near clip plane's center
    glm::vec3 center = position + forward * nearClip;

    float halfWidth = 0.0f, halfHeight = 0.0f;

    if (orthographic)
    {
        // Orthographic projection: fixed size based on orthoSize
        halfWidth = orthoSize * aspectRatio;
        halfHeight = orthoSize;
    }
    else
    {
        // Perspective projection: calculate size based on nearClip and FOV
        halfHeight = nearClip * glm::tan(glm::radians(fov * 0.5f));  // Vertical FOV
        halfWidth = halfHeight * aspectRatio;                        // Horizontal FOV based on aspect ratio
    }

    switch (type)
    {
        case Anchor::TopLeft:
            return center + up * halfHeight - right * halfWidth;
        case Anchor::TopCenter:
            return center + up * halfHeight;
        case Anchor::TopRight:
            return center + up * halfHeight + right * halfWidth;
        case Anchor::CenterLeft:
            return center - right * halfWidth;
        case Anchor::Center:
            return center;
        case Anchor::CenterRight:
            return center + right * halfWidth;
        case Anchor::BottomLeft:
            return center - up * halfHeight - right * halfWidth;
        case Anchor::BottomCenter:
            return center - up * halfHeight;
        case Anchor::BottomRight:
            return center - up * halfHeight + right * halfWidth;
        default:
            return center;
    }
}

float bee::Camera::SmallestDepthDifference() const
{
    // Calculate the number of depth values based on the number of bits in the depth buffer, 12 is just a guess
    int maxDepthValue = (1 << 12) - 1;
    float smallestDepthDifference = (nearClip * farClip) / (maxDepthValue * (farClip - nearClip));
    bee::Log::Info("Smallest depth difference: {0}", smallestDepthDifference);
    return smallestDepthDifference;
}



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/