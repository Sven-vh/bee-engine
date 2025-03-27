#pragma once
#include "core.hpp"

namespace bee
{
class ParticleManager
{
public:
    static void Update(float dt);
    static void FixedUpdate(float dt);
    static void Draw();

    static void CreateEmitter(Emitter emitter, entt::entity entity = entt::null);

private:
    friend class EngineClass;
    static void UpdateEmitters(float dt);
    static void UpdateParticleTransforms(float dt);
    static void UpdateParticleLifetime(float dt);
    static void UpdateParticleColors();

    static void EmitAllParticles(entt::entity emitterEntt);

    static void EmitRemainingParticles(entt::entity emitterEntt);
    static void CreateParticle(entt::entity emitterEntt);

#pragma region Particle Components
    struct ParticlePhysics
    {
        glm::vec3 velocity = glm::vec3(0.0f);
        glm::vec3 angularVelocity = glm::vec3(0.0f);
        glm::vec3 acceleration = glm::vec3(0.0f);
        float rotationSpeed = 0.0f;
    };

    struct LifeTime
    {
        float remaining;
    };

    struct EmitterID
    {
        entt::entity emitterEntity;  // entity id
        // entity id
    };
#pragma endregion
};
}  // namespace bee



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/