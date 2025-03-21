#include "Gameplay.hpp"
#include "bee.hpp"
#include "Components.hpp"
#include "Physics.hpp"

void Gameplay::OnAttach()
{
    auto& registry = bee::Engine.Registry();
    auto path = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::SaveFiles, "PaintGame.scene");
    bee::LoadRegistry(registry, path);

    auto player = bee::ecs::FindEntitiesByName(bee::Engine.Registry(), "Player");
    GAME_EXCEPTION_IF(player.empty(), "No Player found in Scene");
    m_player = player.front();

    auto camera = bee::ecs::FindEntitiesByName(bee::Engine.Registry(), "PlayerView");
    GAME_EXCEPTION_IF(camera.empty(), "No PlayerView found in Scene");
    m_camera = camera.front();
    bee::Engine.SetCamera(m_camera);

    auto gun = bee::ecs::FindEntitiesByName(bee::Engine.Registry(), "Gun");
    GAME_EXCEPTION_IF(gun.empty(), "No Gun found in Scene");
    m_gun = gun.front();

    auto barrel = bee::ecs::FindEntitiesByName(bee::Engine.Registry(), "Barrel");
    GAME_EXCEPTION_IF(barrel.empty(), "No Barrel found in Scene");
    m_barrel = barrel.front();

    auto bulletPrefab = bee::ecs::FindEntitiesByName(bee::Engine.Registry(), "BulletPrefab");
    GAME_EXCEPTION_IF(bulletPrefab.empty(), "No BulletPrefab found in Scene");
    m_bulletPrefab = bulletPrefab.front();

    auto paintSplash = bee::ecs::FindEntitiesByName(bee::Engine.Registry(), "PaintSplashPrefab");
    GAME_EXCEPTION_IF(paintSplash.empty(), "No PaintSplashPrefab found in Scene");
    m_paintSplashPrefab = paintSplash.front();

    auto paintImage = bee::ecs::FindEntitiesByName(bee::Engine.Registry(), "PaintImagePrefab");
    GAME_EXCEPTION_IF(paintImage.empty(), "No PaintImagePrefab found in Scene");
    m_paintImagePrefab = paintImage.front();

    auto earth = bee::ecs::FindEntitiesByName(bee::Engine.Registry(), "Earth");
    GAME_EXCEPTION_IF(earth.empty(), "No Earthfound in Scene");
    m_earth = earth.front();

    auto texturePath = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Assets, "Paint/splat11.png");
    m_paintTextures[0] = bee::resource::LoadResource<bee::resource::Texture>(texturePath);
    // m_paintTextures[1] = bee::resource::LoadResource<bee::resource::Texture>("assets/Paint/paint_splat_b_double.png");
    // m_paintTextures[2] = bee::resource::LoadResource<bee::resource::Texture>("assets/Paint/paint_splat_c_double.png");
    // m_paintTextures[3] = bee::resource::LoadResource<bee::resource::Texture>("assets/Paint/paint_splat_d_double.png");

    bee::Engine.Device().HideCursor(true);
}

void Gameplay::OnDetach() {}

void Gameplay::OnRender() {}

void Gameplay::OnEngineInit() {}

void Gameplay::OnImGuiRender()
{
    ImGui::Begin("Gameplay");
    ImGui::Text("Press 'F1' to show cursor");
    int splashCount = (int)bee::Engine.Registry().view<PaintSplash>().size();

    // clear paint splashes button
    if (ImGui::Button("Clear Paint Splashes"))
    {
        auto view = bee::Engine.Registry().view<PaintSplash>();
        for (auto entity : view)
        {
            bee::ecs::DestroyEntity(entity, bee::Engine.Registry());
        }
    }

    ImGui::Text("Paint Splashes: %d", splashCount);
    ImGui::SliderFloat3("Gun Offset", glm::value_ptr(m_gunOffset), 0.0f, 1.0f, "%.3f");
    ImGui::SliderFloat("Camera Sensitivity", &m_cameraSensitivity, 0.0f, 1.0f, "%.3f");
    ImGui::SliderFloat("Player Speed", &m_playerSpeed, 0.0f, 10.0f, "%.3f");
    ImGui::SliderFloat("Bullets Per Second", &m_bulletPerSecond, 0.0f, 200.0f, "%.3f");
    ImGui::SliderFloat("Bullet Speed", &m_bulletSpeed, 0.0f, 200.0f, "%.3f");
    ImGui::SliderFloat("Bullet Spread", &m_bulletSpread, 0.0f, 45.0f, "%.3f");
    ImGui::SliderFloat("Splash Distance", &m_splashDistance, 0.0f, 1.0f, "%.3f");
    ImGui::SliderFloat("Bullet Lifetime", &m_bulletLifetime, 0.0f, 20.0f, "%.3f");
    ImGui::SliderFloat("Min Bullet Velocity", &m_minBulletVelocity, 0.0f, 1.0f, "%.3f");
    ImGui::Checkbox("Destroy Bullet On Hit", &m_destroyBulletOnHit);
    ImGui::Checkbox("Rainbow Paint", &m_rainbowPaint);
    ImGui::ColorEdit3("Paint Color", &m_paintColor[0]);
    ImGui::End();
}

void Gameplay::OnUpdate(float dt)
{
    UpdateBullets(dt);
    if (!bee::Engine.Device().IsCursorHidden()) return;
    UpdatePosition(dt);
    UpdateCamera();
    UpdateGunPos();

    bee::Transform& earthTransform = bee::Engine.Registry().get<bee::Transform>(m_earth);
    earthTransform.RotateAroundAxis(glm::vec3(0.0, 1.0f, 0.0f), dt * 10);
    glm::vec3 earthPos = earthTransform.GetPosition();
    earthTransform.SetPosition(glm::vec3(earthPos.x, glm::sin(bee::Engine.GetGameTime()), earthPos.z));

    if (m_rainbowPaint)
    {
        m_paintColor = glm::vec3(glm::sin(bee::Engine.GetGameTime()) * 0.5f + 0.5f,
                                 glm::sin(bee::Engine.GetGameTime() + 2.0f) * 0.5f + 0.5f,
                                 glm::sin(bee::Engine.GetGameTime() + 4.0f) * 0.5f + 0.5f);
    }

    // shoot bullets
    static float bulletTimer = 0.0f;
    bulletTimer += dt;

    while (bulletTimer >= 1.0f / m_bulletPerSecond)
    {
        if (bee::Engine.Input().IsMouseAvailable())
        {
            if (bee::Engine.Input().GetMouseButton(bee::Input::MouseButton::Left) && bee::Engine.Device().IsCursorHidden())
            {
                ShootBullet();
            }
        }
        else
        {
            if (bee::Engine.Input().GetGamepadButton(0, bee::Input::GamepadButton::ShoulderRight))
            {
                ShootBullet();
            }
        }
        bulletTimer -= 1.0f / m_bulletPerSecond;
    }
}

void Gameplay::OnFixedUpdate(float) { CheckCollisions(); }

void Gameplay::OnEvent(bee::Event& e)
{
    if (!bee::Engine.Device().IsCursorHidden()) return;

    bee::EventDispatcher dispatcher(e);
    dispatcher.Dispatch<bee::MouseButtonPressedEvent>(BIND_EVENT_FN(Gameplay::OnMousePressed));
}

bool Gameplay::OnMousePressed(bee::MouseButtonPressedEvent& e)
{
    if (e.GetMouseButton() != bee::mouse::Button0)
    {
        return false;
    }

    ShootBullet();

    return true;
}

void Gameplay::ShootBullet()
{
    auto& registry = bee::Engine.Registry();
    entt::entity newBullet = bee::ecs::DuplicateEntity(registry, m_bulletPrefab);
    if (registry.all_of<bee::Disabled>(newBullet))
    {
        registry.remove<bee::Disabled>(newBullet);
    }
    registry.emplace<Bullet>(newBullet, m_bulletLifetime, m_paintColor);
    bee::Transform& bulletTransform = bee::Engine.Registry().get<bee::Transform>(newBullet);
    bulletTransform.SetPosition(bee::GetWorldPosition(m_barrel, bee::Engine.Registry()));

    glm::mat4 barrelWorld = bee::GetWorldModel(m_barrel, registry);
    glm::vec3 barrelForward = glm::vec3(barrelWorld[2]);

    glm::vec3 direction = bee::RandomDirectionInCone(barrelForward, m_bulletSpread);

    Rigidbody& bulletRigidbody = bee::Engine.Registry().get<Rigidbody>(newBullet);
    bulletRigidbody.velocity = direction * m_bulletSpeed;

    bee::Renderable& renderable = registry.get<bee::Renderable>(newBullet);
    renderable.multiplier = glm::vec4(m_paintColor, 1.0f);
    renderable.tint = glm::vec4(0.0f);
}

void Gameplay::UpdateCamera()
{
    auto& input = bee::Engine.Input();
    auto& registry = bee::Engine.Registry();

    bee::Transform& camTransform = registry.get<bee::Transform>(m_camera);

    auto mouse_delta = glm::vec2(0.0f);

    if (bee::Engine.Input().IsMouseAvailable())
    {
        glm::vec2 before = input.GetPreviousMousePosition();
        glm::vec2 after = input.GetMousePosition();
        mouse_delta = after - before;
    }
    else
    {
        float x = input.GetGamepadAxis(0, bee::Input::GamepadAxis::StickRightX);
        float y = input.GetGamepadAxis(0, bee::Input::GamepadAxis::StickRightY);
        mouse_delta = glm::vec2(x, y) / m_cameraSensitivity;
    }

    glm::vec3 newRotation =
        camTransform.GetRotationEuler() - glm::vec3(mouse_delta.y, mouse_delta.x, 0.0f) * m_cameraSensitivity;
    // clamp pitch
    newRotation.x = glm::clamp(newRotation.x, -89.0f, 89.0f);
    camTransform.SetRotation(newRotation);
}

void Gameplay::UpdatePosition(float dt)
{
    auto& registry = bee::Engine.Registry();
    bee::Transform& playerTransform = registry.get<bee::Transform>(m_player);
    bee::Transform& camTransform = registry.get<bee::Transform>(m_camera);

    glm::vec3 moveDir = glm::vec3(0.0f);

    // Get the camera's forward direction but ignore the y-axis for FPS style movement
    glm::vec3 forwardDir = glm::normalize(glm::vec3(camTransform.GetDirection().x, 0.0f, camTransform.GetDirection().z));
    glm::vec3 rightDir = glm::normalize(glm::vec3(camTransform.GetRight().x, 0.0f, camTransform.GetRight().z));

    if (bee::Engine.Input().IsKeyboardAvailable())
    {
        if (bee::Engine.Input().GetKeyboardKey(bee::Input::KeyboardKey::W))
        {
            moveDir += forwardDir;
        }
        if (bee::Engine.Input().GetKeyboardKey(bee::Input::KeyboardKey::S))
        {
            moveDir -= forwardDir;
        }
        if (bee::Engine.Input().GetKeyboardKey(bee::Input::KeyboardKey::A))
        {
            moveDir -= rightDir;
        }
        if (bee::Engine.Input().GetKeyboardKey(bee::Input::KeyboardKey::D))
        {
            moveDir += rightDir;
        }
    }
    else
    {
        float x = bee::Engine.Input().GetGamepadAxis(0, bee::Input::GamepadAxis::StickLeftX);
        float y = bee::Engine.Input().GetGamepadAxis(0, bee::Input::GamepadAxis::StickLeftY);
        moveDir += rightDir * x;
        moveDir += forwardDir * -y;
    }

    if (glm::length(moveDir) > 0.0f)
    {
        moveDir = glm::normalize(moveDir) * m_playerSpeed;
    }

    playerTransform.SetPosition(playerTransform.GetPosition() + moveDir * dt);
}

void Gameplay::UpdateGunPos()
{
    auto& registry = bee::Engine.Registry();
    bee::Camera& cam = registry.get<bee::Camera>(m_camera);
    bee::Transform& camTransform = registry.get<bee::Transform>(m_camera);

    glm::vec3 rightBorderPos = cam.GetAnchorPosition(bee::Anchor::CenterRight,
                                                     bee::GetWorldPosition(m_camera, registry),
                                                     camTransform.GetRotationQuat());
    glm::vec3 leftBorderPos = cam.GetAnchorPosition(bee::Anchor::CenterLeft,
                                                    bee::GetWorldPosition(m_camera, registry),
                                                    camTransform.GetRotationQuat());
    glm::vec3 topBorderPos = cam.GetAnchorPosition(bee::Anchor::TopCenter,
                                                   bee::GetWorldPosition(m_camera, registry),
                                                   camTransform.GetRotationQuat());
    glm::vec3 bottomBorderPos = cam.GetAnchorPosition(bee::Anchor::BottomCenter,
                                                      bee::GetWorldPosition(m_camera, registry),
                                                      camTransform.GetRotationQuat());

    glm::vec3 horizontalOffset = rightBorderPos + (leftBorderPos - rightBorderPos) * m_gunOffset.x;
    glm::vec3 verticalOffset = topBorderPos + (bottomBorderPos - topBorderPos) * m_gunOffset.y;

    glm::vec3 gunPos = horizontalOffset + (verticalOffset - horizontalOffset) * 0.5f;
    gunPos += camTransform.GetDirection() * m_gunOffset.z;

    bee::SetWorldPosition(m_gun, gunPos, registry);
}

void Gameplay::OnBulletHit(entt::entity bullet, entt::entity other, const Physics::CollisionInfo& info)
{
    auto& registry = bee::Engine.Registry();

    if (!registry.valid(bullet))
    {
        return;
    }

    if (info.impactVelocity > m_minBulletVelocity)
    {
        entt::entity newSplash = bee::ecs::DuplicateEntity(registry, m_paintSplashPrefab);
        registry.remove<bee::Disabled>(newSplash);
        bee::Transform& splashTransform = registry.get<bee::Transform>(newSplash);
        splashTransform.SetPosition(info.worldPosition);

        glm::vec3 forward(0.0f, 0.0f, -1.0f);
        glm::quat rotationQuat = glm::rotation(forward, glm::normalize(info.normal));

        splashTransform.SetRotationQuat(rotationQuat);

        glm::vec3 bulletColor = registry.get<Bullet>(bullet).color;

        bee::Emitter& emitter = registry.get<bee::Emitter>(newSplash);
        emitter.particleSpecs.multiplyColorGradient.clear();
        emitter.particleSpecs.multiplyColorGradient.addColor(0.0f, glm::vec4(bulletColor, 1.0f));

        entt::entity newImage = bee::ecs::DuplicateEntity(registry, m_paintImagePrefab);
        registry.remove<bee::Disabled>(newImage);
        bee::Transform& imageTransform = registry.get<bee::Transform>(newImage);
        // Generate a random rotation angle between 0 and 360 degrees
        float randomAngle = glm::radians(glm::linearRand(0.0f, 360.0f));

        // Apply a random rotation around the normal while keeping it aligned as forward direction
        glm::quat randomRotation = glm::angleAxis(randomAngle, info.normal);
        rotationQuat = randomRotation * rotationQuat;  // Combine random rotation with initial orientation

        imageTransform.SetRotationQuat(rotationQuat);

        bee::Renderable& renderable = registry.get<bee::Renderable>(newImage);
        renderable.multiplier = glm::vec4(bulletColor, 1.0f);

        int splashCount = 1 + (int)registry.view<PaintSplash>().size();
        float smallValue = 1e-5f;
        imageTransform.SetPosition(info.worldPosition + info.normal * (((float)splashCount * smallValue + 1e-4f)));

        bee::ecs::SetParentChildRelationship(newImage, other, registry);

        registry.emplace<PaintSplash>(newImage);
    }

    if (m_destroyBulletOnHit) registry.destroy(bullet);
}

void Gameplay::UpdatePaintSplashes() {}

void Gameplay::UpdateBullets(float dt)
{
    auto& registry = bee::Engine.Registry();
    auto bulletView = bee::ecs::GetView<Bullet>(registry);
    for (auto bullet : bulletView)
    {
        Bullet& bulletComponent = registry.get<Bullet>(bullet);
        bulletComponent.lifetime -= dt;

        if (bulletComponent.lifetime <= 0.0f)
        {
            registry.destroy(bullet);
        }
    }
}

void Gameplay::CheckCollisions()
{
    Physics* physics = bee::Engine.GetAppLayer<Physics>();

    auto bulletView = bee::ecs::GetView<Bullet>(bee::Engine.Registry());
    auto colliderView = bee::ecs::GetView<BoxCollider>(bee::Engine.Registry());

    for (auto bullet : bulletView)
    {
        for (auto collider : colliderView)
        {
            if (bullet == collider)
            {
                continue;
            }
            Physics::CollisionInfo info;
            if (physics->HasCollided(bullet, collider, info))
            {
                OnBulletHit(bullet, collider, info);
            }
        }
    }
}
