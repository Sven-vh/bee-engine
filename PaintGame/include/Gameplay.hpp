#pragma once
#include "bee.hpp"
#include "Physics.hpp"

class Gameplay : public bee::Layer
{
public:
    Gameplay() : Layer("Gameplay") {}
    ~Gameplay() = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnRender() override;
    void OnEngineInit() override;
    void OnImGuiRender() override;
    void OnUpdate(float deltaTime) override;
    void OnFixedUpdate(float deltaTime) override;
    void OnEvent(bee::Event& e) override;

private:
    entt::entity m_player;
    entt::entity m_camera;
    entt::entity m_gun;
    entt::entity m_bulletPrefab;
    entt::entity m_paintSplashPrefab;
    entt::entity m_paintImagePrefab;
    entt::entity m_barrel;
    entt::entity m_earth;

    std::array<Ref<bee::resource::Texture>, 4> m_paintTextures;

    glm::vec3 m_gunOffset = glm::vec3(-0.5f, 1.5f, 0.25f);

    float m_cameraSensitivity = 0.1f;
    float m_playerSpeed = 10.0f;
    float m_bulletSpeed = 25.0f;
    float m_splashDistance = 0.1f;
    float m_bulletLifetime = 10.0f;
    float m_minBulletVelocity = 1.0f;
    float m_bulletSpread = 10.0f;
    float m_bulletPerSecond = 60.0f;

    bool m_rainbowPaint = true;
    glm::vec3 m_paintColor = glm::vec3(1.0f, 0.0f, 0.0f);

    bool m_destroyBulletOnHit = false;

private:
    void UpdateCamera();
    void UpdateGunPos();

    void UpdatePosition(float dt);

    bool OnMousePressed(bee::MouseButtonPressedEvent& e);

    void ShootBullet();

    void CheckCollisions();
    void OnBulletHit(entt::entity bullet, entt::entity collider, const Physics::CollisionInfo& info);
    void UpdatePaintSplashes();
    void UpdateBullets(float dt);
};


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/