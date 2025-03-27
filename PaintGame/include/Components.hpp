#pragma once
#include "common.hpp"

struct Bullet
{
    float lifetime = 10.0f;
    glm::vec3 color;

    template <typename Archive>
    void serialize(Archive& archive)
    {
        make_optional_nvp(archive, "lifetime", lifetime);
    }
};

struct Rigidbody
{
    glm::vec3 velocity;
    glm::vec3 acceleration;
    glm::vec3 previousPosition;

    template <typename Archive>
    void serialize(Archive& archive)
    {
        make_optional_nvp(archive, "velocity", velocity);
        make_optional_nvp(archive, "acceleration", acceleration);
    }
};

struct SphereCollider
{
    float radius;
    bool isTrigger;

    template <typename Archive>
    void serialize(Archive& archive)
    {
        make_optional_nvp(archive, "radius", radius);
        make_optional_nvp(archive, "isTrigger", isTrigger);
    }
};

struct BoxCollider
{
    glm::vec3 size = glm::vec3(1.0f);
    bool isTrigger = false;

    template <typename Archive>
    void serialize(Archive& archive)
    {
        make_optional_nvp(archive, "isTrigger", isTrigger);
        make_optional_nvp(archive, "size", size);
    }
};

struct PaintSplash
{
};

#ifdef BEE_PLATFORM_PC
#pragma warning(push)
#pragma warning(disable : 4505)
#endif

// imgui function
static void DrawBoxCollider(entt::entity entity)
{
    auto& registry = bee::Engine.Registry();
    auto& boxCollider = registry.get<BoxCollider>(entity);

    ImGui::Checkbox("Is Trigger", &boxCollider.isTrigger);
    ImGui::DragFloat3("Size", &boxCollider.size[0], 0.1f);
}

#ifdef BEE_PLATFORM_PC
#pragma warning(pop)
#endif


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/