#include "ApplicationLayer.h"
#include "bee.hpp"

static void DrawTileComponent(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    if (ImGui::TreeNodeEx("Tile", DEFAULT_TREENODE_FLAGS))
    {
        auto& tile = registry.get<ApplicationLayer::Tile>(entity);
        ImGui::InputInt("X", &tile.x);
        ImGui::InputInt("Y", &tile.y);
        ImGui::Checkbox("Is Colored", &tile.isColored);

        ImGui::TreePop();
    }
}

static void DrawCubeSideComponent(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    if (ImGui::TreeNodeEx("CubeSide", DEFAULT_TREENODE_FLAGS))
    {
        auto& cubeSide = registry.get<ApplicationLayer::CubeSide>(entity);
        ImGui::Checkbox("Is Colored", &cubeSide.isColored);

        ImGui::TreePop();
    }
}

void ApplicationLayer::OnEngineInit()
{
    bee::Log::Info("Registering Tile component");
    bee::ComponentManager::RegisterComponent<Tile, &DrawTileComponent>("Tile", true, true);

    bee::Log::Info("Registering CubeSide component");
    bee::ComponentManager::RegisterComponent<CubeSide, &DrawCubeSideComponent>("CubeSide", true, true);

    // create entity with Tile component
    // auto& registry = bee::Engine.Registry();
    // auto entity = registry.create();

    // registry.emplace<Tile>(entity, 1, 2, true);
    // registry.emplace<CubeSide>(entity, false);
}

void ApplicationLayer::OnAttach()
{
    auto& registry = bee::Engine.Registry();

    // Load the scene
    auto path = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::SaveFiles, "TileLevel.scene");
    bee::LoadRegistry(registry, path);

    // Get all the tiles in the scene
    m_tiles = bee::ecs::GetEntitiesWithComponent<Tile, bee::Renderable>(bee::Engine.Registry());
    GAME_EXCEPTION_IF(m_tiles.empty(), "No Tile found in Scene");

    // loop through the tile components and give them a random color
    // for (auto& tile : m_tiles)
    //{
    //    auto& tileComponent = registry.get<Tile>(tile);
    //    tileComponent.isColored = rand() % 2;
    //    auto& renderable = registry.get<bee::Renderable>(tile);
    //    renderable.tint = tileComponent.isColored ? glm::vec4(1, 0, 0, 1) : glm::vec4(0, 0, 0, 1);
    //}

    // pick 6 random tiles and color them
    for (int i = 0; i < 6; i++)
    {
        int randomIndex = rand() % m_tiles.size();
        auto& tileComponent = registry.get<Tile>(m_tiles[randomIndex]);
        tileComponent.isColored = true;
        auto& renderable = registry.get<bee::Renderable>(m_tiles[randomIndex]);
        renderable.tint = glm::vec4(1, 0, 0, 1);
    }

    // Get the player entity
    auto player = bee::ecs::FindEntitiesByName(bee::Engine.Registry(), "Player");
    GAME_EXCEPTION_IF(player.empty(), "No Player found in Scene");
    m_player = player.front();

    m_cubeSides = bee::ecs::GetEntitiesWithComponent<CubeSide, bee::Renderable>(bee::Engine.Registry());
    GAME_EXCEPTION_IF(m_cubeSides.empty(), "No CubeSide found in Scene");

    // get camera entity
    camUp = bee::ecs::FindEntitiesByName(bee::Engine.Registry(), "CamUp").front();
    bee::Engine.SetCamera(camUp);
}

void ApplicationLayer::OnDetach() { m_tiles.clear(); }

void ApplicationLayer::OnUpdate(float )
{
    auto& cameraTransform = bee::Engine.Registry().get<bee::Transform>(bee::Engine.MainCamera());
    glm::vec3 cameraPos = cameraTransform.GetPosition();

    float fixedY = cameraPos.y;

    if (bee::Engine.Input().GetMouseButton(bee::Input::MouseButton::Right))
    {
        glm::vec2 delta = bee::Engine.Input().GetMousePosition() - bee::Engine.Input().GetPreviousMousePosition();

        float radius = glm::length(glm::vec2(cameraPos.x, cameraPos.z));
        float azimuth = atan2(cameraPos.z, cameraPos.x);

        float sensitivity = 0.005f;
        azimuth += delta.x * sensitivity;

        cameraPos.x = radius * cos(azimuth);
        cameraPos.z = radius * sin(azimuth);
        cameraPos.y = fixedY;

        // Set the new position of the camera
        cameraTransform.SetPosition(cameraPos);

        glm::mat4 newMat = bee::helper::LookToPosition(cameraTransform.GetModelMatrix(), glm::vec3(0.0f), false);

        cameraTransform.SetModelMatrix(newMat);
    }
}

void ApplicationLayer::OnFixedUpdate(float) {}

void ApplicationLayer::OnImGuiRender()
{
    ImGui::Begin("Application Window");
    // dropdown for m_rotationEase
    bee::ImGuiHelper::EnumCombo("Rotation Ease", m_rotationEase);
    ImGui::End();
}

void ApplicationLayer::OnEvent(bee::Event& e)
{
    // On Key Pressed event
    bee::EventDispatcher dispatcher(e);
    dispatcher.Dispatch<bee::KeyPressedEvent>(BIND_EVENT_FN(ApplicationLayer::OnKeyPressed));
    dispatcher.Dispatch<bee::MouseButtonPressedEvent>(BIND_EVENT_FN(ApplicationLayer::OnMouseButtonPressed));
}

void ApplicationLayer::MovePlayer(glm::int2 direction)
{
    if (m_isTweening) return;

    glm::vec3 movementDirection = GetGroundPlaneMovementDirection(direction);
    if (glm::length(movementDirection) == 0.0f) return;

    float cubeSize = 2.0f;
    glm::vec3 movement = movementDirection * cubeSize;

    auto& m_playerTransform = bee::Engine.Registry().get<bee::Transform>(m_player);
    glm::vec3 initialPosition = m_playerTransform.GetPosition();
    glm::vec3 targetPosition = initialPosition + movement;
    glm::quat initialRotation = m_playerTransform.GetRotationQuat();

    glm::vec3 rotationAxis = CalculateRotationAxis(movementDirection);
    glm::vec3 pivotPoint = CalculatePivotPoint(initialPosition, movement, cubeSize);

    StartPlayerMovementTween(m_playerTransform, initialPosition, targetPosition, pivotPoint, rotationAxis, initialRotation);
    m_isTweening = true;
}

glm::vec3 ApplicationLayer::GetGroundPlaneMovementDirection(glm::int2 direction)
{
    entt::entity cameraEntity = bee::Engine.MainCamera();
    auto& cameraTransform = bee::Engine.Registry().get<bee::Transform>(cameraEntity);

    glm::vec3 cameraForward = GetGroundPlaneDirection(cameraTransform.GetDirection());
    glm::vec3 cameraRight = GetGroundPlaneDirection(cameraTransform.GetRight());

    glm::vec3 movementDirection =
        cameraForward * static_cast<float>(direction.y) + cameraRight * static_cast<float>(direction.x);
    if (glm::length(movementDirection) != 0.0f)
    {
        return PickDominantAxisDirection(glm::normalize(movementDirection));
    }
    return glm::vec3(0.0f);
}

glm::vec3 ApplicationLayer::GetGroundPlaneDirection(glm::vec3 direction)
{
    direction.y = 0;  // Ignore vertical component for ground movement
    return glm::normalize(direction);
}

glm::vec3 ApplicationLayer::PickDominantAxisDirection(glm::vec3 movementDirection)
{
    return (glm::abs(movementDirection.x) > glm::abs(movementDirection.z))
               ? glm::vec3((movementDirection.x > 0 ? 1 : -1), 0, 0)
               : glm::vec3(0, 0, (movementDirection.z > 0 ? 1 : -1));
}

glm::vec3 ApplicationLayer::CalculateRotationAxis(const glm::vec3& movementDirection)
{
    return -glm::cross(movementDirection, glm::vec3(0, 1, 0));  // Cross product with up vector to get rotation axis
}

glm::vec3 ApplicationLayer::CalculatePivotPoint(const glm::vec3& initialPosition, const glm::vec3& movement, float cubeSize)
{
    return initialPosition + (movement * 0.5f) - glm::vec3(0, cubeSize / 2.0f, 0);
}

void ApplicationLayer::StartPlayerMovementTween(bee::Transform& m_playerTransform,
                                                const glm::vec3& initialPosition,
                                                const glm::vec3& targetPosition,
                                                const glm::vec3& pivotPoint,
                                                const glm::vec3& rotationAxis,
                                                const glm::quat& initialRotation)
{
    auto tween = CREATE_TWEEN(float, 0.0f, glm::half_pi<float>(), 0.2f);
    tween->OnUpdate(
        [&m_playerTransform, initialPosition, rotationAxis, pivotPoint, initialRotation](const float& angle, float )
        {
            glm::quat rotationQuat = glm::angleAxis(angle, rotationAxis);
            glm::vec3 offset = initialPosition - pivotPoint;
            glm::vec3 rotatedOffset = rotationQuat * offset;
            glm::vec3 newPosition = pivotPoint + rotatedOffset;

            m_playerTransform.SetPosition(newPosition);
            m_playerTransform.SetRotationQuat(rotationQuat * initialRotation);
        });

    tween->OnFinish(
        [this, &m_playerTransform, targetPosition](const float& )
        {
            m_playerTransform.SetPosition(targetPosition);
            m_isTweening = false;
            CheckTiles(targetPosition);
        });

    tween->SetEase(m_rotationEase);
    bee::Tweener::AddTween(tween);
}

void ApplicationLayer::CheckTiles(const glm::vec3& targetPosition)
{
    for (auto& tile : m_tiles)
    {
        auto& tileTransform = bee::Engine.Registry().get<bee::Transform>(tile);
        auto roundedPosition = glm::round(targetPosition);
        auto tilePosition = glm::vec3(tileTransform.GetPosition().x, 0, tileTransform.GetPosition().z);
        if (roundedPosition.x == tilePosition.x && roundedPosition.z == tilePosition.z)
        {
            Tile& tileComponent = bee::Engine.Registry().get<Tile>(tile);
            bee::Renderable& tileRenderable = bee::Engine.Registry().get<bee::Renderable>(tile);

            entt::entity lowestSide = GetLowestSide();
            CubeSide& lowestSideComponent = bee::Engine.Registry().get<CubeSide>(lowestSide);
            bee::Renderable& sideRenderable = bee::Engine.Registry().get<bee::Renderable>(lowestSide);

            bool isSideColored = lowestSideComponent.isColored;
            bool isTileColored = tileComponent.isColored;

            if (isSideColored && isTileColored)
            {
                // do nothing
                break;
            }
            else if (isSideColored && !isTileColored)
            {
                tileComponent.isColored = true;
                tileRenderable.tint = glm::vec4(1, 0, 0, 1);

                lowestSideComponent.isColored = false;
                sideRenderable.tint = glm::vec4(0, 0, 0, 1);

                break;
            }
            else if (!isSideColored && isTileColored)
            {
                tileComponent.isColored = false;
                tileRenderable.tint = glm::vec4(0, 0, 0, 1);

                lowestSideComponent.isColored = true;
                sideRenderable.tint = glm::vec4(1, 0, 0, 1);

                break;
            }
            else if (!isSideColored && !isTileColored)
            {
                // do nothing
                break;
            }

            // tileComponent.isColored = !tileComponent.isColored;
            // auto& renderable = bee::Engine.Registry().get<bee::Renderable>(tile);
            // renderable.tint = tileComponent.isColored ? glm::vec4(1, 0, 0, 1) : glm::vec4(0, 0, 0, 1);
            break;
        }
    }
}

entt::entity ApplicationLayer::GetLowestSide()
{
    // loop through the cube sides and get the lowest one
    entt::entity lowestSide = m_cubeSides.front();
    auto& registry = bee::Engine.Registry();

    glm::vec3 lowesPosition = bee::GetWorldPosition(lowestSide, registry);

    for (auto& side : m_cubeSides)
    {
        glm::vec3 sidePosition = bee::GetWorldPosition(side, registry);
        if (sidePosition.y < lowesPosition.y)
        {
            lowestSide = side;
            lowesPosition = sidePosition;
        }
    }

    return lowestSide;
}

bool ApplicationLayer::OnKeyPressed(bee::KeyPressedEvent& e)
{
    // switch case for the arrows
    switch (e.GetKeyCode())
    {
        case bee::key::Up:
            MovePlayer({0, 1});
            return true;
        case bee::key::Down:
            MovePlayer({0, -1});
            return true;
        case bee::key::Left:
            MovePlayer({-1, 0});
            return true;
        case bee::key::Right:
            MovePlayer({1, 0});
            return true;
        default:
            break;
    }

    // swithc cam with 1, 2, 3
    switch (e.GetKeyCode())
    {
        case bee::key::D1:
            bee::Engine.SetCamera(camUp);
            return true;
        case bee::key::D2:
            bee::Engine.SetCamera(camDiag);
            return true;
        case bee::key::D3:
            bee::Engine.SetCamera(camFront);
            return true;
        default:
            break;
    }

    return false;
}

bool ApplicationLayer::OnMouseButtonPressed(bee::MouseButtonPressedEvent& ) { return false; }
