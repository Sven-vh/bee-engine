#include "managers/particle_manager.hpp"
#include "core.hpp"

void bee::ParticleManager::Update(float dt)
{
    UpdateParticleLifetime(dt);
    UpdateEmitters(dt);
    UpdateParticleColors();
}

void bee::ParticleManager::FixedUpdate(float dt) { UpdateParticleTransforms(dt); }

void bee::ParticleManager::UpdateEmitters(float dt)
{
    auto& registry = bee::Engine.Registry();
    auto view = bee::ecs::GetView<Emitter>(registry);
    for (const auto& entity : view)
    {
        auto& emitter = view.get<Emitter>(entity);

        emitter.time += dt;

        if (emitter.specs.lifetime > 0.0f && emitter.time >= emitter.specs.lifetime && bee::Engine.IsPlaying())
        {
            registry.destroy(entity);
            continue;
        }

        if (emitter.specs.active)
        {
            if (emitter.specs.loop)
            {
                if (emitter.specs.burst)
                {
                    if (emitter.particleCount == 0)
                    {
                        EmitAllParticles(entity);
                    }
                }
                else
                {
                    EmitRemainingParticles(entity);
                }
            }
            else if (emitter.time == dt)
            {
                EmitAllParticles(entity);
            }
        }
    }
}

void bee::ParticleManager::EmitRemainingParticles(entt::entity emitterEntt)
{
    auto& registry = bee::Engine.Registry();
    auto& emitter = registry.get<Emitter>(emitterEntt);
    // calculate the number of particles to emit based on the emission rate and the emitter time
    int particlesToEmit =
        emitter.specs.burst ? (int)emitter.specs.spawnCountPerSecond : (int)(emitter.time * emitter.specs.spawnCountPerSecond);
    emitter.time -= particlesToEmit / emitter.specs.spawnCountPerSecond;
    // emitter.time = glm::max(emitter.time, 0.0f);

    for (int i = 0; i < particlesToEmit; i++)
    {
        if (emitter.particleCount >= emitter.specs.maxParticles)
        {
            break;
        }

        CreateParticle(emitterEntt);
    }
}

void bee::ParticleManager::UpdateParticleTransforms(float dt)
{
    // Process particles
    auto& registry = bee::Engine.Registry();
    auto view = bee::ecs::GetView<Transform, ParticlePhysics, LifeTime>(registry);
    for (auto entity : view)
    {
        auto& transform = view.get<Transform>(entity);
        auto& particlePhysics = view.get<ParticlePhysics>(entity);

        // Update particle velocity and position
        if (dt > 0.0f)
        {
            particlePhysics.velocity += particlePhysics.acceleration * dt;
            transform.SetPosition(transform.GetPosition() + particlePhysics.velocity * dt);
        }
        else
        {
            transform.SetPosition(transform.GetPosition() + particlePhysics.velocity * dt);
            particlePhysics.velocity += particlePhysics.acceleration * dt;
        }
        transform.SetRotation(transform.GetRotationEuler() +
                              particlePhysics.angularVelocity * particlePhysics.rotationSpeed * dt);
    }
}

void bee::ParticleManager::UpdateParticleLifetime(float dt)
{
    auto& registry = bee::Engine.Registry();
    auto view = bee::ecs::GetView<LifeTime>(registry);
    for (auto entity : view)
    {
        // Retrieve the components of the entity
        auto& lifetime = view.get<LifeTime>(entity);
        // Update particle lifetime
        lifetime.remaining -= dt;
        if (lifetime.remaining <= 0.0f)
        {
            auto& emitterID = registry.get<EmitterID>(entity);
            entt::entity emitterEntity = emitterID.emitterEntity;
            if (registry.valid(emitterEntity) && registry.all_of<Emitter>(emitterEntity))
            {
                auto& emitter = registry.get<Emitter>(emitterEntity);
                emitter.particleCount--;
            }

            registry.destroy(entity);
        }
    }
}

void bee::ParticleManager::UpdateParticleColors()
{
    auto& registry = bee::Engine.Registry();
    auto view = bee::ecs::GetView<Transform, LifeTime, Renderable, EmitterID>(registry);
    for (auto entity : view)
    {
        auto& emitterID = view.get<EmitterID>(entity);
        entt::entity emitterEntity = emitterID.emitterEntity;

        const bool validEmitter = registry.valid(emitterEntity) && registry.all_of<Emitter>(emitterEntity);
        if (!validEmitter) continue;
        Emitter emitter = registry.get<Emitter>(emitterEntity);
        if (validEmitter) emitter = registry.get<Emitter>(emitterEntity);

        if (!emitter.specs.active) continue;

        auto& transform = view.get<Transform>(entity);
        const auto& lifetime = view.get<LifeTime>(entity);
        auto& renderable = view.get<Renderable>(entity);

        const float life = 1.0f - lifetime.remaining / emitter.particleSpecs.maxLifetime;
        const float size =
            easeLerp(emitter.particleSpecs.startSize, emitter.particleSpecs.endSize, life, emitter.particleSpecs.sizeEase);
        transform.SetScale(glm::vec3(size));

        if (!emitter.particleSpecs.randomColor)
        {
            if (emitter.particleSpecs.multiplyColor)
            {
                renderable.multiplier = emitter.particleSpecs.multiplyColorGradient.getColor(life);
            }
            else
            {
                renderable.multiplier = glm::vec4(1.0f);
            }

            if (emitter.particleSpecs.addColor)
            {
                renderable.tint = emitter.particleSpecs.addColorGradient.getColor(life);
            }
            else
            {
                renderable.tint = glm::vec4(0.0f);
            }
        }
    }
}

void bee::ParticleManager::EmitAllParticles(entt::entity emitterEntt)
{
    // destroy all particles
    auto& registry = bee::Engine.Registry();
    auto view = bee::ecs::GetView<ParticlePhysics, LifeTime, EmitterID>(registry);

    Emitter& emitter = registry.get<Emitter>(emitterEntt);

    for (auto entity : view)
    {
        auto& emitterID = view.get<EmitterID>(entity);
        entt::entity emitterEntity = emitterID.emitterEntity;
        if (registry.valid(emitterEntity) && registry.all_of<Emitter>(emitterEntity))
        {
            auto& newEmitter = registry.get<Emitter>(emitterEntity);
            if (newEmitter.id == emitter.id)
            {
                registry.destroy(entity);
                newEmitter.particleCount--;
            }
        }
        else
        {
            registry.destroy(entity);
        }
    }

    // Create new particles
    EmitRemainingParticles(emitterEntt);
}

void bee::ParticleManager::CreateParticle(entt::entity emitterEntt)
{
    auto& registry = bee::Engine.Registry();
    auto newParticle = bee::ecs::CreateEmpty();
    glm::vec3 velocity;
    auto& emitter = registry.get<Emitter>(emitterEntt);
    auto& transform = registry.get<Transform>(emitterEntt);
    // check if the emitter has a renderable component
    if (!registry.all_of<Renderable>(emitterEntt))
    {
        // add a default renderable component
        Renderable render;
        render.visible = false;
        registry.emplace<Renderable>(emitterEntt, render);
        bee::Log::Warn("Emitter does not have a renderable component. Creating a default renderable component");
    }
    auto& render = registry.get<Renderable>(emitterEntt);

    if (transform.GetRotationEuler() == glm::vec3(0.0f))
    {
        transform.SetRotation(glm::vec3(0.0f, 1.0f, 0.0f));
    }

    if (emitter.particleSpecs.randomVelocity)
    {
        if (emitter.specs.useWorldSpace)
        {
            glm::mat4 worldTransform = bee::GetWorldModel(emitterEntt, registry);
            glm::vec3 worldDirection = glm::vec3(worldTransform * glm::vec4(transform.GetDirection(), 0.0f));
            velocity =
                RandomDirectionInCone(worldDirection, emitter.specs.coneSpecs.angle) * emitter.particleSpecs.startVelocity;
        }
        else
        {
            velocity = RandomDirectionInCone(transform.GetDirection(), emitter.specs.coneSpecs.angle) *
                       emitter.particleSpecs.startVelocity;
        }
    }
    else
    {
        if (emitter.specs.useWorldSpace)
        {
            glm::mat4 worldTransform = bee::GetWorldModel(emitterEntt, registry);
            glm::vec3 worldDirection = glm::vec3(worldTransform * glm::vec4(transform.GetDirection(), 0.0f));
            velocity = worldDirection * emitter.particleSpecs.startVelocity;
        }
        else
        {
            velocity = transform.GetDirection() * emitter.particleSpecs.startVelocity;
        }
    }

    glm::vec3 startRotation;
    if (emitter.particleSpecs.randomStartRotation)
    {
        startRotation = RandomDirectionInSphere();
    }
    else
    {
        startRotation = emitter.particleSpecs.startRotation;
    }

    glm::vec3 angularVelocity;
    if (emitter.particleSpecs.randomRotate)
    {
        angularVelocity = RandomDirectionInSphere();
    }
    else
    {
        angularVelocity = startRotation;
    }

    registry.emplace<ParticlePhysics>(newParticle,
                                      velocity,
                                      angularVelocity,
                                      emitter.particleSpecs.acceleration,
                                      emitter.particleSpecs.rotationSpeed);

    glm::vec3 position = GetWorldPosition(emitterEntt, registry);
    registry.emplace<Transform>(newParticle, Transform(position, startRotation, glm::vec3(emitter.particleSpecs.startSize)));

    float lifetime = emitter.particleSpecs.randomLifetime
                         ? RandomFloat(emitter.particleSpecs.minLifetime, emitter.particleSpecs.maxLifetime)
                         : emitter.particleSpecs.startLifeTime;
    registry.emplace<LifeTime>(newParticle, lifetime);

    glm::vec4 multiplyColor;

    glm::vec4 addColor;
    if (emitter.particleSpecs.randomColor)
    {
        multiplyColor = glm::vec4(RandomFloat(0.0f, 1.0f), RandomFloat(0.0f, 1.0f), RandomFloat(0.0f, 1.0f), 1.0f);
        addColor = glm::vec4(RandomFloat(0.0f, 1.0f), RandomFloat(0.0f, 1.0f), RandomFloat(0.0f, 1.0f), 1.0f);
    }
    else
    {
        if (emitter.particleSpecs.multiplyColor)
        {
            multiplyColor = emitter.particleSpecs.multiplyColorGradient.getColor(0.0f);
        }
        else
        {
            multiplyColor = glm::vec4(1.0f);
        }

        if (emitter.particleSpecs.addColor)
        {
            addColor = emitter.particleSpecs.addColorGradient.getColor(0.0f);
        }
        else
        {
            addColor = glm::vec4(0.0f);
        }
    }
    registry.emplace<Renderable>(
        newParticle,
        Renderable(render.mesh, render.texture, multiplyColor, addColor, true, render.billboard, render.receiveShadows));

    registry.emplace<EmitterID>(newParticle, emitterEntt);
    emitter.particleCount++;
}

void bee::ParticleManager::Draw()
{
    auto& registry = bee::Engine.Registry();
    auto emitterView = bee::ecs::GetView<Emitter, Transform>(registry);
    for (auto entity : emitterView)
    {
        auto& emitter = emitterView.get<Emitter>(entity);
        auto& transform = emitterView.get<Transform>(entity);
        if (emitter.specs.coneSpecs.debugDraw)
        {
            glm::mat4 worldTransform = bee::GetWorldModel(entity, registry);
            // get position from the world transform
            auto worldPosition = glm::vec3(worldTransform[3]);
            auto direction = glm::vec3(worldTransform * glm::vec4(transform.GetDirection(), 0.0f));
            bee::DebugRenderer::DrawCone(worldPosition,
                                         direction,
                                         emitter.specs.coneSpecs.angle,
                                         glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
    }

    // for each particle draw an debug arrow to show the direction of the particle and color based on the velocity
    auto particleView = bee::ecs::GetView<ParticlePhysics, Transform, EmitterID>(registry);
    for (auto entity : particleView)
    {
        auto& emitterID = particleView.get<EmitterID>(entity);
        entt::entity emitterEntity = emitterID.emitterEntity;

        bool validEmitter = registry.valid(emitterEntity) && registry.all_of<Emitter>(emitterEntity);
        Emitter emitter;
        if (validEmitter) emitter = registry.get<Emitter>(emitterEntity);

        if (validEmitter && emitter.specs.active && emitter.particleSpecs.drawVelocity)
        {
            auto& transform = particleView.get<Transform>(entity);
            auto& particlePhysics = particleView.get<ParticlePhysics>(entity);

            float velocityMagnitude = glm::length(particlePhysics.velocity);

            float minVelocity = 0.0f;                                              // Minimum velocity (green)
            float maxVelocity = glm::length(emitter.particleSpecs.startVelocity);  // Maximum velocity (red)

            float velocityNormalized = glm::clamp((velocityMagnitude - minVelocity) / (maxVelocity - minVelocity), 0.0f, 1.0f);

            glm::vec3 color = glm::mix(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), velocityNormalized);

            bee::DebugRenderer::DrawArrow(transform.GetPosition(), particlePhysics.velocity, 1.0f, glm::vec4(color, 1.0f));
        }
    }
}

void bee::ParticleManager::CreateEmitter(Emitter emitter, entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    if (entity == entt::null)
    {
        entity = bee::ecs::CreateDefault();
    }
    emitter.id = rand();

    // check if the entity already has a renderable component
    if (registry.all_of<Renderable>(entity))
    {
        // load the mesh and texture form the path
        Renderable& render = registry.get<Renderable>(entity);
        std::string data = bee::Engine.FileIO().ReadTextFile(bee::FileIO::Directory::Root, render.meshPath);
        render.mesh->GetHandle() = xsr::tools::load_obj_mesh(data, "");
        if (!render.mesh->is_valid())
        {
            bee::Log::Error("Loading mesh failed. Creating a new mesh");
            render.mesh = bee::resource::LoadResource<bee::resource::Mesh>(
                bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_MODEL));
        }

        std::vector<char> textureData = bee::Engine.FileIO().ReadBinaryFile(bee::FileIO::Directory::None, render.texturePath);
        render.texture->GetHandle() = xsr::tools::load_png_texture(textureData);
        if (!render.texture->IsValid())
        {
            bee::Log::Error("Loading texture failed. Creating a new texture");
            render.texture = bee::resource::LoadResource<bee::resource::Texture>(
                bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_TEXTURE));
        }
    }
    else
    {
        // create a new mesh and texture
        // xsr::mesh_handle mesh = xsr::create_default_cube_mesh();
        Ref<bee::resource::Mesh> mesh = bee::resource::LoadResource<bee::resource::Mesh>(
            bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_MODEL));

        Ref<bee::resource::Texture> texture = bee::resource::LoadResource<bee::resource::Texture>(
            bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_TEXTURE));

        registry.emplace<Renderable>(entity, Renderable(mesh, texture, glm::vec4(1.0f), glm::vec4(0.0f), false));
    }

    Transform transform;
    if (transform.GetRotationEuler() == glm::vec3(0.0f))
    {
        transform.SetRotation(glm::vec3(0.0f, 1.0f, 0.0f));
    }

    // check if the entity already has a emitter component
    if (registry.all_of<Emitter>(entity))
    {
        registry.replace<Emitter>(entity, emitter);
    }
    else
    {
        registry.emplace<Emitter>(entity, emitter);
    }
}