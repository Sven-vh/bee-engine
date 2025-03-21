#include "rendering/PerspectiveCamera.hpp"
#include "core.hpp"

bee::PerspectiveCamera::PerspectiveCamera(glm::vec3 position,
                                          glm::vec3 rotation,
                                          float fov,
                                          float aspectRatio,
                                          float nearClip,
                                          float farClip,
                                          float cameraSpeed)
    : m_position(position),
      m_rotation(rotation),
      m_fov(fov),
      m_aspectRatio(aspectRatio),
      m_nearClip(nearClip),
      m_farClip(farClip),
      m_cameraSpeed(cameraSpeed)
{
    m_projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
    RecalculateViewMatrix();
}
glm::vec3 bee::PerspectiveCamera::GetForward() const
{
    return glm::normalize(glm::vec3(m_viewMatrix[0][2], m_viewMatrix[1][2], m_viewMatrix[2][2]));
}

glm::vec3 bee::PerspectiveCamera::GetUp() const
{
    return glm::normalize(glm::vec3(m_viewMatrix[0][1], m_viewMatrix[1][1], m_viewMatrix[2][1]));
}

glm::vec3 bee::PerspectiveCamera::GetRight() const
{
    return glm::normalize(glm::vec3(m_viewMatrix[0][0], m_viewMatrix[1][0], m_viewMatrix[2][0]));
}

void bee::PerspectiveCamera::SetAspectRatio(float aspectRatio)
{
    this->m_aspectRatio = aspectRatio;
    RecalculateViewMatrix();
}

void bee::PerspectiveCamera::Update(float deltatime)
{
    bee::Input& input = bee::Engine.Input();

    // if right mouse button is pressed then move else not
    if (!input.GetMouseButton(bee::Input::MouseButton::Right))
    {
        return;
    }

    // update camera based on input
    if (input.GetKeyboardKey(bee::Input::KeyboardKey::W))
    {
        SetPosition(GetPosition() - GetForward() * m_cameraSpeed * deltatime);
    }
    if (input.GetKeyboardKey(bee::Input::KeyboardKey::S))
    {
        SetPosition(GetPosition() + GetForward() * m_cameraSpeed * deltatime);
    }
    if (input.GetKeyboardKey(bee::Input::KeyboardKey::A))
    {
        SetPosition(GetPosition() - GetRight() * m_cameraSpeed * deltatime);
    }
    if (input.GetKeyboardKey(bee::Input::KeyboardKey::D))
    {
        SetPosition(GetPosition() + GetRight() * m_cameraSpeed * deltatime);
    }
    if (input.GetKeyboardKey(bee::Input::KeyboardKey::Q))
    {
        SetPosition(GetPosition() - GetUp() * m_cameraSpeed * deltatime);
    }
    if (input.GetKeyboardKey(bee::Input::KeyboardKey::E))
    {
        SetPosition(GetPosition() + GetUp() * m_cameraSpeed * deltatime);
    }
    m_cameraSpeed *= pow(1.5f, input.GetMouseWheel());

    // if right mouse button is pressed, rotate the camera
    if (input.GetMouseButton(bee::Input::MouseButton::Right))
    {
        glm::vec2 before = input.GetPreviousMousePosition();
        glm::vec2 after = input.GetMousePosition();

        glm::vec2 mouse_delta = after - before;
        SetRotation(GetRotation() - glm::vec3(mouse_delta.y, mouse_delta.x, 0.0f) * 0.005f);
    }
}

bee::raycasting::Ray bee::PerspectiveCamera::ScreenPointToRay(const glm::vec2& screenPoint) const
{
    return ScreenPointToRay(screenPoint, (float)bee::Engine.Device().GetWidth(), (float)bee::Engine.Device().GetHeight());
}

bee::raycasting::Ray bee::PerspectiveCamera::ScreenPointToRay(const glm::vec2& screenPoint,
                                             const float screenWidth,
                                             const float screenHeight) const
{
    glm::vec2 normalizedCoords(screenPoint.x / screenWidth * 2.0f - 1.0f, 1.0f - screenPoint.y / screenHeight * 2.0f);

    glm::vec4 clipCoords(normalizedCoords.x, normalizedCoords.y, -1.0f, 1.0f);

    glm::mat4 invProj = glm::inverse(m_projectionMatrix);
    glm::vec4 eyeCoords = invProj * clipCoords;
    eyeCoords = glm::vec4(eyeCoords.x, eyeCoords.y, -1.0f, 0.0f);

    glm::mat4 invView = glm::inverse(m_viewMatrix);
    glm::vec3 rayWorld = glm::vec3(invView * eyeCoords);

    if (glm::length(rayWorld) == 0) return bee::raycasting::Ray(m_position, glm::vec3(0.0f));  // Handle zero-length ray

    rayWorld = glm::normalize(rayWorld);
    return bee::raycasting::Ray(m_position, rayWorld);
}

void bee::PerspectiveCamera::RecalculateViewMatrix()
{
    // Create the rotation matrix using yaw (Y) and pitch (X) only
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), m_rotation.y, glm::vec3(0, 1, 0)) *  // Yaw
                               glm::rotate(glm::mat4(1.0f), m_rotation.x, glm::vec3(1, 0, 0));   // Pitch

    // Apply the rotation to the camera's forward direction
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_position) * rotationMatrix;

    m_projectionMatrix = glm::perspective(glm::radians(m_fov), m_aspectRatio, m_nearClip, m_farClip);

    m_viewMatrix = glm::inverse(transform);
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
}