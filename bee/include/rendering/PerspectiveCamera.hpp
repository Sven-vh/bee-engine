#pragma once

#include "common.hpp"
#include "tools/raycasting.hpp"

namespace bee
{
class PerspectiveCamera
{
public:
    PerspectiveCamera(){};
    PerspectiveCamera(glm::vec3 position,
                      glm::vec3 rotation,
                      float fov,
                      float aspectRatio,
                      float nearClip,
                      float farClip,
                      float cameraSpeed = 10.0f);

    const glm::mat4& GetProjectionMatrix() const { return m_projectionMatrix; }
    const glm::mat4& GetViewMatrix() const { return m_viewMatrix; }
    const glm::mat4& GetViewProjectionMatrix() const { return m_viewProjectionMatrix; }

    const glm::vec3& GetPosition() const { return m_position; }
    void SetPosition(const glm::vec3& position)
    {
        this->m_position = position;
        RecalculateViewMatrix();
    }

    const glm::vec3& GetRotation() const { return m_rotation; }
    void SetRotation(const glm::vec3& rotation)
    {
        this->m_rotation = rotation;
        RecalculateViewMatrix();
    }

    glm::vec3 GetForward() const;
    glm::vec3 GetUp() const;
    glm::vec3 GetRight() const;

    float GetFov() const { return m_fov; }
    void SetFov(float fov)
    {
        this->m_fov = fov;
        RecalculateViewMatrix();
    }

    float GetAspectRatio() const { return m_aspectRatio; }
    void SetAspectRatio(float aspectRatio);

    float GetNearClip() const { return m_nearClip; }
    void SetNearClip(float nearClip)
    {
        this->m_nearClip = nearClip;
        RecalculateViewMatrix();
    }

    float GetFarClip() const { return m_farClip; }
    void SetFarClip(float farClip)
    {
        this->m_farClip = farClip;
        RecalculateViewMatrix();
    }

    float& GetSpeed() { return m_cameraSpeed; }
    void SetSpeed(float speed) { m_cameraSpeed = speed; }

    float GetYaw() const { return m_rotation.y; }
    float GetPitch() const { return m_rotation.x; }

    void Update(float deltatime);

    raycasting::Ray ScreenPointToRay(const glm::vec2& screenPoint) const;
    raycasting::Ray ScreenPointToRay(const glm::vec2& screenPoint, const float screenWidth, const float screenHeight) const;

private:
    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_viewProjectionMatrix;

    glm::vec3 m_position = {0.0f, 0.0f, 0.0f};
    glm::vec3 m_rotation = {0.0f, 0.0f, 0.0f};

    float m_fov, m_aspectRatio, m_nearClip, m_farClip, m_cameraSpeed;

    void RecalculateViewMatrix();
};
}  // namespace bee
