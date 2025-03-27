#pragma once
#include "bee.hpp"

class CarGame : public bee::Layer
{
public:
    struct Player
    {
        float speed = 0.0f;
        float maxSpeed = 5.0f;               // Max speed in units/second
        float acceleration = 5.0f;            // Acceleration in units/second
        float deceleration = 4.0f;            // Deceleration in units/second (reverse)
        float friction = 3.0f;                // Friction for slowing down in units/second
        float maxSteeringAngle = 3.0f;        // Max steering angle in radians (approx. 28.6 degrees)
        float steerSpeed = 10.0f;             // Steering speed in radians/second
        float brakePower = 4.0f;              // Braking force in units/second
        float steeringAngle = 0.0f;           // Current steering angle
        float minSpeedForSteering = 0.1f;     // Min speed to allow steering
        float returnToCenterSpeed = 4.0f;     // How quickly the steering returns to center in radians/second
        float lowSpeedSteeringPower = 100.0f;  // Exponent for scaling steering at low speeds
        float minSteeringFactor = 0.5f;       // Minimum steering factor at high speeds

        template <class Archive>
        void save(Archive& archive) const
        {
            archive(cereal::make_nvp("speed", speed));
        }

        template <class Archive>
        void load(Archive& archive)
        {
            archive(cereal::make_nvp("speed", speed));
        }
    };

    struct Tire
    {
        bool isBreaking;

        template <class Archive>
        void save(Archive& archive) const
        {
            archive(cereal::make_nvp("isBreaking", isBreaking));
        }

        template <class Archive>
        void load(Archive& archive)
        {
            archive(cereal::make_nvp("isBreaking", isBreaking));
        }
    };

public:
    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float deltaTime) override;
    void MoveCamera();
    void OnEngineInit() override;
    void OnFixedUpdate(float deltaTime) override;
    void OnImGuiRender() override;
    void OnEvent(bee::Event& e) override;

private:
    entt::entity m_player;
    std::vector<entt::entity> m_tires;

    float m_tireSpeed = 100.0f;
    glm::vec3 m_cameraOffset = glm::vec3(0.0f, 0.2f, 0.0f);
    

    bee::EaseType m_sideTiltingEase = bee::EaseType::EaseOutQuart;
    float m_maxSideTiltingAngle = 5.0f;

    bee::EaseType m_forwardsTiltingEase = bee::EaseType::EaseOutQuart;
    float m_maxForwardsTiltingAngle = 5.0f;

private:
    void MovePlayer(entt::entity& player, float deltaTime);
    void ApplyFriction(Player& playerComponent, float deltaTime);
    void GoForwards(entt::entity& player, float deltaTime);
    void GoBackwards(entt::entity& player, float deltaTime);
    void SteerLeft(entt::entity& player, float deltaTime, float axis);
    void SteerRight(entt::entity& player, float deltaTime, float axis);
    void Brake(entt::entity& player, float deltaTime);
    void UpdateCarVisual();
    void HandleInput(float deltaTime);
};


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/