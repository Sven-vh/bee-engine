#include "CarGame.h"

void CarGame::OnEngineInit()
{
    bee::ComponentManager::RegisterComponentNoDraw<CarGame::Player>("Player", true, true);
    bee::ComponentManager::RegisterComponentNoDraw<CarGame::Tire>("Tire", true, true);
}

void CarGame::OnAttach()
{
    auto& registry = bee::Engine.Registry();

    // Load the scene
    auto path = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::SaveFiles, "CarGame.scene");
    bee::LoadRegistry(registry, path);

    m_tires = bee::ecs::GetEntitiesWithComponent<Tire, bee::Renderable>(registry);
    GAME_EXCEPTION_IF(m_tires.empty(), "No Tile found in Scene");

    // Get the player entity
    auto players = bee::ecs::GetEntitiesWithComponent<Player>(registry);
    GAME_EXCEPTION_IF(players.empty(), "No Player found in Scene");
    m_player = players.front();

    // get the camera entity
    auto cameras = bee::ecs::GetEntitiesWithComponent<bee::Camera>(registry);
    GAME_EXCEPTION_IF(cameras.empty(), "No Camera found in Scene");
    auto camera = cameras.front();
    bee::Engine.SetCamera(camera);

    bee::Engine.Device().HideCursor(true);
}

void CarGame::OnDetach() {}

void CarGame::OnUpdate(float deltaTime)
{
    auto& player = bee::Engine.Registry().get<Player>(m_player);
    // rotate the tires
    for (auto tire : m_tires)
    {
        auto& transform = bee::Engine.Registry().get<bee::Transform>(tire);
        transform.SetRotation(transform.GetRotationEuler() + glm::vec3(m_tireSpeed, 0.0f, 0.0f) * player.speed * deltaTime);
    }

    HandleInput(deltaTime);

    // Move the player
    MovePlayer(m_player, deltaTime);
    UpdateCarVisual();
}

// most of the driving code is from ChatGPT
void CarGame::UpdateCarVisual()
{
    // Get the car visual transform
    auto& player = bee::Engine.Registry().get<Player>(m_player);

    entt::entity carVisual = bee::Engine.Registry().get<bee::HierarchyNode>(m_player).children.front();
    auto& visualTransform = bee::Engine.Registry().get<bee::Transform>(carVisual);

    // --- Side tilting (Z-axis) ---
    float tiltFactor = player.steeringAngle / player.maxSteeringAngle;  // Get tiltFactor in range [-1, 1]
    float tiltSign = glm::sign(tiltFactor);  // Store the sign of the steering angle (positive or negative)
    tiltFactor = glm::abs(tiltFactor);       // Take the absolute value for easing (in range [0, 1])

    auto sideEaseFunc = bee::getEaseFunction(m_sideTiltingEase);
    tiltFactor = sideEaseFunc(tiltFactor);  // Apply the easing function
    tiltFactor *= tiltSign;                 // Restore the original sign after easing

    float tiltAngleZ = tiltFactor * glm::radians(m_maxSideTiltingAngle);  // Apply the tilt factor to the max angle

    // --- Forward/Backward tilting (X-axis) ---
    float speedFactor = glm::clamp(
        glm::abs(player.speed) / player.maxSpeed,
        0.0f,
        1.0f);  // Speed factor between 0 and 1                                           // Preserve the sign of the speed
    auto forwardEaseFunc = bee::getEaseFunction(m_forwardsTiltingEase);
    speedFactor = forwardEaseFunc(speedFactor);
    speedFactor *= glm::sign(player.speed);
    float tiltAngleX = speedFactor * glm::radians(m_maxForwardsTiltingAngle);  // Lean back when going fast

    // Combine both tilt angles
    glm::quat tiltRotationZ = glm::angleAxis(tiltAngleZ, glm::vec3(0.0f, 0.0f, 1.0f));   // Side tilt on Z-axis
    glm::quat tiltRotationX = glm::angleAxis(-tiltAngleX, glm::vec3(1.0f, 0.0f, 0.0f));  // Forward/Backward tilt on X-axis

    // Apply both tilt rotations to the car visual
    visualTransform.SetRotationQuat(tiltRotationX * tiltRotationZ);
}

void CarGame::HandleInput(float deltaTime)
{
    if (!bee::Engine.Device().IsCursorHidden()) return;

    // Handle input
    if (bee::Engine.Input().GetKeyboardKey(bee::Input::KeyboardKey::W) ||
        bee::Engine.Input().GetGamepadButton(0, bee::Input::GamepadButton::ShoulderRight))
    {
        GoForwards(m_player, deltaTime);
    }
    if (bee::Engine.Input().GetKeyboardKey(bee::Input::KeyboardKey::S) ||
        bee::Engine.Input().GetGamepadButton(0, bee::Input::GamepadButton::ShoulderLeft))
    {
        GoBackwards(m_player, deltaTime);
    }
    if (bee::Engine.Input().IsKeyboardAvailable())
    {
        if (bee::Engine.Input().GetKeyboardKey(bee::Input::KeyboardKey::A))
        {
            SteerRight(m_player, deltaTime, 1.0f);
        }
        if (bee::Engine.Input().GetKeyboardKey(bee::Input::KeyboardKey::D))
        {
            SteerLeft(m_player, deltaTime, 1.0f);
        }
    }
    else if (bee::Engine.Input().IsGamepadAvailable(0))
    {
        float x = bee::Engine.Input().GetGamepadAxis(0, bee::Input::GamepadAxis::StickLeftX);
        if (glm::abs(x) > 0.1f)
        {
            if (x > 0)
                SteerLeft(m_player, deltaTime, glm::abs(x));
            else
                SteerRight(m_player, deltaTime, glm::abs(x));
        }
    }
    if (bee::Engine.Input().GetKeyboardKey(bee::Input::KeyboardKey::Space) ||
        bee::Engine.Input().GetGamepadButton(0, bee::Input::GamepadButton::South))
    {
        Brake(m_player, deltaTime);
    }

    MoveCamera();
}

void CarGame::MoveCamera()
{
    auto& cameraTransform = bee::Engine.Registry().get<bee::Transform>(bee::Engine.MainCamera());
    glm::vec3 cameraPos = cameraTransform.GetPosition();

    float radius = glm::length(cameraPos);

    glm::vec2 delta(0.0f);
    if (bee::Engine.Input().IsMouseAvailable())
    {
        delta = bee::Engine.Input().GetMousePosition() - bee::Engine.Input().GetPreviousMousePosition();
    }
    else if (bee::Engine.Input().IsGamepadAvailable(0))
    {
        float x = bee::Engine.Input().GetGamepadAxis(0, bee::Input::GamepadAxis::StickRightX);
        float y = bee::Engine.Input().GetGamepadAxis(0, bee::Input::GamepadAxis::StickRightY);
        delta = glm::vec2(x, y) * 5.0f;
    }

    float azimuth = atan2(cameraPos.z, cameraPos.x);
    float elevation = asin(cameraPos.y / radius);

    float sensitivity = 0.005f;
    azimuth += delta.x * sensitivity;
    elevation += delta.y * sensitivity;

    constexpr float minElevation = glm::radians(10.0f);
    constexpr float maxElevation = glm::radians(89.0f);
    elevation = glm::clamp(elevation, minElevation, maxElevation);

    // Convert spherical coordinates back to Cartesian
    cameraPos.x = radius * cos(elevation) * cos(azimuth);
    cameraPos.y = radius * sin(elevation);
    cameraPos.z = radius * cos(elevation) * sin(azimuth);

    // Set the new position of the camera
    cameraTransform.SetPosition(cameraPos);

    glm::mat4 newMat = bee::helper::LookToPosition(cameraTransform.GetModelMatrix(), m_cameraOffset, false);
    cameraTransform.SetModelMatrix(newMat);
}

void CarGame::OnFixedUpdate(float) {}

void CarGame::OnImGuiRender()
{
    ImGui::Begin("Car Game");
    // settings for the player
    auto& playerComponent = bee::Engine.Registry().get<Player>(m_player);
    ImGui::Text("Player Settings");
    ImGui::Text("Speed: %.2f", playerComponent.speed);
    ImGui::SliderFloat("Max Speed", &playerComponent.maxSpeed, 0.0f, 100.0f);
    ImGui::SliderFloat("Acceleration", &playerComponent.acceleration, 0.0f, 100.0f);
    ImGui::SliderFloat("Deceleration", &playerComponent.deceleration, 0.0f, 100.0f);
    ImGui::SliderFloat("Friction", &playerComponent.friction, 0.0f, 100.0f);
    ImGui::Text("Steering Angle: %.2f", playerComponent.steeringAngle);
    ImGui::SliderFloat("Max Steering Angle", &playerComponent.maxSteeringAngle, 0.0f, 360.0f);
    ImGui::SliderFloat("Steer Speed", &playerComponent.steerSpeed, 0.0f, 360.0f);
    ImGui::SliderFloat("Brake Power", &playerComponent.brakePower, 0.0f, 100.0f);
    ImGui::SliderFloat("Min Speed For Steering", &playerComponent.minSpeedForSteering, 0.0f, 100.0f);
    ImGui::SliderFloat("Return To Center Speed", &playerComponent.returnToCenterSpeed, 0.0f, 100.0f);
    ImGui::SliderFloat("Low Speed Steering Multiplier", &playerComponent.lowSpeedSteeringPower, 0.0f, 10.0f);
    ImGui::SliderFloat("Min Steering Factor", &playerComponent.minSteeringFactor, 0.0f, 1.0f);

    // settings for the tires
    ImGui::Text("Tire Settings");
    ImGui::SliderFloat("Tire Speed", &m_tireSpeed, 0.0f, 2.0f);

    // m_cameraOffset
    ImGui::Text("Camera Settings");
    ImGui::DragFloat3("Camera Offset X", &m_cameraOffset.x);

    // easing functions
    ImGui::Text("Easing Functions");
    ImGui::Text("Side Tilting");
    ImGui::SliderFloat("Max Side Tilting Angle", &m_maxSideTiltingAngle, 0.0f, 45.0f);
    bee::ImGuiHelper::EnumCombo("Side Tilting Ease", m_sideTiltingEase);
    ImGui::Text("Forwards Tilting");
    ImGui::SliderFloat("Max Forwards Tilting Angle", &m_maxForwardsTiltingAngle, 0.0f, 45.0f);
    bee::ImGuiHelper::EnumCombo("Forwards Tilting Ease", m_forwardsTiltingEase);

    ImGui::End();
}

void CarGame::OnEvent(bee::Event& ) {}

// most of the driving code is from ChatGPT
void CarGame::MovePlayer(entt::entity& player, float deltaTime)
{
    auto& playerComponent = bee::Engine.Registry().get<Player>(player);
    auto& transform = bee::Engine.Registry().get<bee::Transform>(player);

    // Apply steering only when the car is moving
    if (glm::abs(playerComponent.speed) > 0.0f)  // Allow steering at any speed > 0, and we'll fade it smoothly
    {
        glm::vec3 direction = -transform.GetDirection();

        // Smoothly scale steering effectiveness based on speed using a power function
        float normalizedSpeed = glm::abs(playerComponent.speed) / playerComponent.maxSpeed;

        // Apply power curve: at low speeds, steering is sharper, at high speeds it becomes less sharp
        float steeringFactor = glm::pow(1.0f - normalizedSpeed, playerComponent.lowSpeedSteeringPower);

        // Ensure steeringFactor doesn't fall below the minimum steering factor at high speeds
        steeringFactor = glm::max(steeringFactor, playerComponent.minSteeringFactor);

        // Smoothly fade out steering as speed approaches minSpeedForSteering
        float fadeSteeringFactor = glm::smoothstep(playerComponent.minSpeedForSteering, 0.5f, glm::abs(playerComponent.speed));

        // Combine the steering factor with the fade factor for smooth steering control at low speeds
        steeringFactor *= fadeSteeringFactor;

        // Adjust steering amount based on the adjusted steering factor
        float steerAmount = playerComponent.steeringAngle * deltaTime * steeringFactor;

        glm::quat steerRotation = glm::angleAxis(steerAmount, transform.GetUp());

        // Apply steering to direction
        glm::vec3 velocity = steerRotation * direction * playerComponent.speed;
        glm::vec3 newPos = transform.GetPosition() + velocity * deltaTime;
        transform.SetPosition(newPos);

        // Update rotation based on steering
        glm::quat newRot = transform.GetRotationQuat() * steerRotation;
        transform.SetRotationQuat(newRot);
    }
    else
    {
        // If the car is not moving, just apply the current direction
        glm::vec3 velocity = -transform.GetDirection() * playerComponent.speed;
        glm::vec3 newPos = transform.GetPosition() + velocity * deltaTime;
        transform.SetPosition(newPos);
    }

    // Gradually return the steering angle back to zero if no steering input is active
    if (playerComponent.steeringAngle > 0)
    {
        playerComponent.steeringAngle -= playerComponent.returnToCenterSpeed * deltaTime;
        if (playerComponent.steeringAngle < 0) playerComponent.steeringAngle = 0;
    }
    else if (playerComponent.steeringAngle < 0)
    {
        playerComponent.steeringAngle += playerComponent.returnToCenterSpeed * deltaTime;
        if (playerComponent.steeringAngle > 0) playerComponent.steeringAngle = 0;
    }

    // Apply friction when not accelerating or braking
    ApplyFriction(playerComponent, deltaTime);
}

void CarGame::ApplyFriction(Player& playerComponent, float deltaTime)
{
    if (glm::abs(playerComponent.speed) > 0)
    {
        float friction = playerComponent.friction * deltaTime;

        // Slow down based on friction
        if (playerComponent.speed > 0)
        {
            playerComponent.speed -= friction;
            if (playerComponent.speed < 0) playerComponent.speed = 0;
        }
        else if (playerComponent.speed < 0)
        {
            playerComponent.speed += friction;
            if (playerComponent.speed > 0) playerComponent.speed = 0;
        }
    }
}

void CarGame::GoForwards(entt::entity& player, float deltaTime)
{
    auto& playerComponent = bee::Engine.Registry().get<Player>(player);
    playerComponent.speed += playerComponent.acceleration * deltaTime;
    if (playerComponent.speed > playerComponent.maxSpeed) playerComponent.speed = playerComponent.maxSpeed;
}

void CarGame::GoBackwards(entt::entity& player, float deltaTime)
{
    auto& playerComponent = bee::Engine.Registry().get<Player>(player);
    playerComponent.speed -= playerComponent.deceleration * deltaTime;
    if (playerComponent.speed < -playerComponent.maxSpeed) playerComponent.speed = -playerComponent.maxSpeed;
}

void CarGame::SteerLeft(entt::entity& player, float deltaTime, float axis)
{
    auto& playerComponent = bee::Engine.Registry().get<Player>(player);
    if (glm::abs(playerComponent.speed) > playerComponent.minSpeedForSteering)
    {
        playerComponent.steeringAngle -= playerComponent.steerSpeed * deltaTime * axis;
        if (playerComponent.steeringAngle < -playerComponent.maxSteeringAngle)
            playerComponent.steeringAngle = -playerComponent.maxSteeringAngle;
    }
}

void CarGame::SteerRight(entt::entity& player, float deltaTime, float axis)
{
    auto& playerComponent = bee::Engine.Registry().get<Player>(player);
    if (glm::abs(playerComponent.speed) > playerComponent.minSpeedForSteering)
    {
        playerComponent.steeringAngle += playerComponent.steerSpeed * deltaTime * axis;
        if (playerComponent.steeringAngle > playerComponent.maxSteeringAngle)
            playerComponent.steeringAngle = playerComponent.maxSteeringAngle;
    }
}

void CarGame::Brake(entt::entity& player, float deltaTime)
{
    auto& playerComponent = bee::Engine.Registry().get<Player>(player);
    if (playerComponent.speed > 0)
    {
        playerComponent.speed -= playerComponent.brakePower * deltaTime;
        if (playerComponent.speed < 0) playerComponent.speed = 0;
    }
    else if (playerComponent.speed < 0)
    {
        playerComponent.speed += playerComponent.brakePower * deltaTime;
        if (playerComponent.speed > 0) playerComponent.speed = 0;
    }
}



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/