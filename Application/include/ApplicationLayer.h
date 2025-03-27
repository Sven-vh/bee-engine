#pragma once
#include "bee.hpp"

class ApplicationLayer : public bee::Layer
{
public:
    struct CubeSide
    {
        bool isColored;

        template <class Archive>
        void serialize(Archive& archive)
        {
            archive(isColored);
        }

        template <class Archive>
        void deserialize(Archive& archive)
        {
            archive(isColored);
        }
    };

    struct Tile
    {
        int x, y;
        bool isColored;

        template <class Archive>
        void serialize(Archive& archive)
        {
            archive(x, y, isColored);
        }

        template <class Archive>
        void deserialize(Archive& archive)
        {
            archive(x, y, isColored);
        }
    };

public:
    ApplicationLayer() : Layer("ApplicationLayer") {}
    virtual ~ApplicationLayer() = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float deltaTime) override;
    void OnEngineInit() override;
    void OnFixedUpdate(float deltaTime) override;
    void OnImGuiRender() override;
    void OnEvent(bee::Event& e) override;

private:
    entt::entity m_player;
    bee::EaseType m_rotationEase = bee::EaseType::EaseOutQuad;
    bool m_isTweening = false;

    std::vector<entt::entity> m_tiles;
    std::vector<entt::entity> m_cubeSides;

    entt::entity camUp;
    entt::entity camFront;
    entt::entity camDiag;

private:
    void MovePlayer(glm::int2 direction);

    glm::vec3 GetGroundPlaneMovementDirection(glm::int2 direction);

    glm::vec3 GetGroundPlaneDirection(glm::vec3 direction);

    glm::vec3 PickDominantAxisDirection(glm::vec3 movementDirection);

    glm::vec3 CalculateRotationAxis(const glm::vec3& movementDirection);

    glm::vec3 CalculatePivotPoint(const glm::vec3& initialPosition, const glm::vec3& movement, float cubeSize);

    void StartPlayerMovementTween(bee::Transform& m_playerTransform,
                                  const glm::vec3& initialPosition,
                                  const glm::vec3& targetPosition,
                                  const glm::vec3& pivotPoint,
                                  const glm::vec3& rotationAxis,
                                  const glm::quat& initialRotation);

    void CheckTiles(const glm::vec3& targetPosition);
    entt::entity GetLowestSide();

    bool OnKeyPressed(bee::KeyPressedEvent& e);
    bool OnMouseButtonPressed(bee::MouseButtonPressedEvent& e);
};



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/