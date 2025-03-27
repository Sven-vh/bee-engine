#pragma once
#include "common.hpp"
#include <xsr/include/xsr.hpp>
#include "core/fileio.hpp"
#include "tools/gradient.hpp"
#include "tools/ease.hpp"
#include "tools/raycasting.hpp"
#include "resource/resourceManager.hpp"
#include "resource/gltfLoader.hpp"
#include "core/engine.hpp"
#include "rendering/FrameBuffer.hpp"
#include "cereal/cereal_optional_nvp.h"
#include "tools/cerealHelper.hpp"
#include "defines.hpp"

namespace bee
{

class Components;  // this is just here so I can easily find the components

struct HierarchyNode
{
    std::string name;
    entt::entity parent = entt::null;
    bool selectParent = false;
    std::vector<entt::entity> children = {};

    HierarchyNode() = default;
    HierarchyNode(std::string& n) noexcept : name(std::move(n)) {}
    HierarchyNode(const std::string& n) noexcept : name(n) {}

    void DestroyChildren(entt::registry& registry);

    template <class Archive>
    void serialize(Archive& archive)
    {
        // archive(cereal::make("name", name),
        //         cereal::make_nvp("parent", parent),
        //         cereal::make_nvp("children", children),
        //         cereal::make_nvp("selectParent", selectParent));
        make_optional_nvp(archive, "name", name);
        make_optional_nvp(archive, "parent", parent);
        make_optional_nvp(archive, "children", children);
        make_optional_nvp(archive, "selectParent", selectParent);
    }
};

struct EditorComponent
{
};

struct Saveable
{
};

struct Disabled
{
    template <class Archive>
    void serialize(Archive& archive)
    {
        // archive(cereal::make_nvp("disabled", disabled));
        make_optional_nvp(archive, "disabled", disabled);
    }

private:
    bool disabled = false;
};

struct Transform
{
public:
    Transform() { m_dirty = true; }
    Transform(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scl);

    void SetPosition(const glm::vec3& pos);
    void SetRotationQuat(const glm::quat& rot);
    void SetRotation(const glm::vec3& euler);
    void SetScale(const glm::vec3& scl);

    void RotateAroundAxis(const glm::vec3& axis, float angleDegrees);

    glm::vec3 GetPosition() const { return position; }
    glm::quat GetRotationQuat() const { return rotation; }
    glm::vec3 GetRotationEuler() const { return rotationEuler; }
    glm::vec3 GetScale() const { return scale; }

    glm::vec3 GetDirection() const;
    glm::vec3 GetRight() const;
    glm::vec3 GetUp() const;

    void SetDirty(bool dirty) { m_dirty = dirty; }

    template <class Archive>
    void save(Archive& archive) const
    {
        make_optional_nvp(archive, "position", position);
        make_optional_nvp(archive, "rotation", rotation);
        make_optional_nvp(archive, "scale", scale);
        make_optional_nvp(archive, "rotationEuler", rotationEuler);
    }

    template <class Archive>
    void load(Archive& archive)
    {
        make_optional_nvp(archive, "position", position);
        make_optional_nvp(archive, "rotation", rotation);
        make_optional_nvp(archive, "scale", scale);
        make_optional_nvp(archive, "rotationEuler", rotationEuler);
        m_dirty = true;
    }

    const glm::mat4& GetModelMatrix();

    void SetModelMatrix(const glm::mat4& modelMatrix);

private:
    friend class ComponentManager;

    glm::mat4 m_model = glm::mat4(1.0f);
    bool m_dirty = false;

    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(glm::vec3(0.0f));
    glm::vec3 scale = glm::vec3(1.0f);
    glm::vec3 rotationEuler = glm::vec3(0.0f);
};

#define DEFAULT_MULTIPLY_COLOR glm::vec4(1.0f)
#define DEFAULT_TINT_COLOR glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)

struct Renderable
{
    Ref<bee::resource::Mesh> mesh;
    fs::path meshPath = "";
    Ref<bee::resource::Texture> texture;
    fs::path texturePath = "";
    glm::vec4 multiplier = DEFAULT_MULTIPLY_COLOR;
    glm::vec4 tint = DEFAULT_TINT_COLOR;
    bool visible = true;
    bool billboard = false;
    bool receiveShadows = true;

    Renderable();
    Renderable(const Ref<bee::resource::Mesh>& mesh,
               const Ref<bee::resource::Texture>& texture,
               const glm::vec4& multiplier,
               const glm::vec4& tint,
               bool visible = true,
               bool billboard = false,
               bool receive_shadows = true);

    template <class Archive>
    void save(Archive& archive) const
    {
        // archive(cereal::make_nvp("meshPath", meshPath),
        //         cereal::make_nvp("texturePath", texturePath),
        //         cereal::make_nvp("multiplier", multiplier),
        //         cereal::make_nvp("tint", tint),
        //         cereal::make_nvp("visible", visible),
        //         cereal::make_nvp("billboard", billboard));
        make_optional_nvp(archive, "meshPath", meshPath.string());
        make_optional_nvp(archive, "texturePath", texturePath.string());
        make_optional_nvp(archive, "multiplier", multiplier);
        make_optional_nvp(archive, "tint", tint);
        make_optional_nvp(archive, "visible", visible);
        make_optional_nvp(archive, "billboard", billboard);
        make_optional_nvp(archive, "receiveShadows", receiveShadows);
    }

    template <class Archive>
    void load(Archive& archive)
    {
        // archive(cereal::make_nvp("meshPath", meshPath),
        //         cereal::make_nvp("texturePath", texturePath),
        //         cereal::make_nvp("multiplier", multiplier),
        //         cereal::make_nvp("tint", tint),
        //         cereal::make_nvp("visible", visible),
        //         cereal::make_nvp("billboard", billboard));

        std::string oldMeshPath = "";
        make_optional_nvp(archive, "meshPath", oldMeshPath);
        meshPath = oldMeshPath;

        std::string oldTexturePath = "";
        make_optional_nvp(archive, "texturePath", oldTexturePath);
        texturePath = oldTexturePath;

        make_optional_nvp(archive, "multiplier", multiplier);
        make_optional_nvp(archive, "tint", tint);
        make_optional_nvp(archive, "visible", visible);
        make_optional_nvp(archive, "billboard", billboard);
        make_optional_nvp(archive, "receiveShadows", receiveShadows);

        bool isMeshPathEmpty = meshPath.empty();
        bool isTexturePathEmpty = texturePath.empty();
        if (isMeshPathEmpty)
        {
            mesh = bee::resource::LoadResource<bee::resource::Mesh>(
                bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_MODEL));
        }
        if (isTexturePathEmpty)
        {
            texture = bee::resource::LoadResource<bee::resource::Texture>(
                bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_TEXTURE));
        }

        if (isTexturePathEmpty && isMeshPathEmpty)
        {
            return;
        }

        if (!isMeshPathEmpty)
        {
            mesh = bee::resource::LoadResource<bee::resource::Mesh>(
                bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Root, meshPath));
        }

        if (!isTexturePathEmpty)
        {
            texture = bee::resource::LoadResource<bee::resource::Texture>(
                bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Root, texturePath));
        }

        if (!mesh.get() || !mesh->is_valid())
        {
            mesh = bee::resource::LoadResource<bee::resource::Mesh>(
                bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_MODEL));
        }

        if (!texture.get() || !texture->IsValid())
        {
            texture = bee::resource::LoadResource<bee::resource::Texture>(
                bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_TEXTURE));
        }
    }
};

struct Raycastable
{
    bool raycastable = true;

    template <class Archive>
    void serialize(Archive& archive)
    { /*
         archive(cereal::make_nvp("raycastable", raycastable));*/
        make_optional_nvp(archive, "raycastable", raycastable);
    }
};

struct EditorIcon
{
    Ref<bee::resource::Mesh> quad;
    Ref<bee::resource::Texture> icon;
    fs::path iconPath = "";

    EditorIcon();
    EditorIcon(const fs::path& iconPath);

    template <class Archive>
    void save(Archive& archive) const
    {
        // archive(cereal::make_nvp("iconPath", iconPath));
        make_optional_nvp(archive, "iconPath", iconPath);
    }

    template <class Archive>
    void load(Archive& archive)
    {
        // archive(cereal::make_nvp("iconPath", iconPath));
        make_optional_nvp(archive, "iconPath", iconPath);

        if (iconPath.empty())
        {
            icon = bee::resource::LoadResource<bee::resource::Texture>(
                bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_TEXTURE));
            return;
        }

        icon = bee::resource::LoadResource<bee::resource::Texture>(
            bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, iconPath));
    }
};

struct PointLight
{
    glm::vec4 color = glm::vec4(1.0f);
    float range = 10.0f;

    template <class Archive>
    void serialize(Archive& archive)
    {
        // archive(cereal::make_nvp("color", color), cereal::make_nvp("range", range));
        make_optional_nvp(archive, "color", color);
        make_optional_nvp(archive, "range", range);
    }
};

struct DirectionalLight
{
    glm::vec4 color = glm::vec4(1.0f);

    template <class Archive>
    void serialize(Archive& archive)
    {
        // archive(cereal::make_nvp("color", color));
        make_optional_nvp(archive, "color", color);
    }
};

struct SceneData
{
    ColorGradient skyColor;
    glm::vec4 ambientColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);

    SceneData();

    template <class Archive>
    void serialize(Archive& archive)
    {
        // archive(cereal::make_nvp("skyColor", skyColor), cereal::make_nvp("ambientColor", ambientColor));
        make_optional_nvp(archive, "skyColor", skyColor);
        make_optional_nvp(archive, "ambientColor", ambientColor);
    }
};

#pragma region ParticleSystem
struct ConeSpecs
{
    float angle = 30.0f;
    bool debugDraw = true;

    template <class Archive>
    void serialize(Archive& archive)
    {
        // archive(cereal::make_nvp("angle", angle), cereal::make_nvp("debugDraw", debugDraw));
        make_optional_nvp(archive, "angle", angle);
        make_optional_nvp(archive, "debugDraw", debugDraw);
    }
};

struct EmitterSpecs
{
    float lifetime = -1.0f;
    float spawnCountPerSecond = 1;
    int maxParticles = 100;
    bool active = true;
    bool loop = true;
    bool burst = false;
    bool useWorldSpace = false;
    ConeSpecs coneSpecs;

    template <class Archive>
    void serialize(Archive& archive)
    {
        // archive(cereal::make_nvp("lifetime", lifetime),
        //         cereal::make_nvp("spawnCount", spawnCount),
        //         cereal::make_nvp("maxParticles", maxParticles),
        //         cereal::make_nvp("active", active),
        //         cereal::make_nvp("loop", loop),
        //         cereal::make_nvp("burst", burst),
        //         cereal::make_nvp("coneSpecs", coneSpecs));
        make_optional_nvp(archive, "lifetime", lifetime);
        make_optional_nvp(archive, "spawnCountPerSecond", spawnCountPerSecond);
        make_optional_nvp(archive, "maxParticles", maxParticles);
        make_optional_nvp(archive, "active", active);
        make_optional_nvp(archive, "loop", loop);
        make_optional_nvp(archive, "burst", burst);
        make_optional_nvp(archive, "coneSpecs", coneSpecs);
        make_optional_nvp(archive, "useWorldSpace", useWorldSpace);
    }
};

struct ParticleSpecs
{
    float startSize = 0.1f, endSize = 0.1f;
    EaseType sizeEase = EaseType::Linear;

    bool randomStartRotation = false;
    bool randomRotate = false;
    bool randomColor = false;
    bool multiplyColor = false;
    glm::vec3 startRotation = glm::vec3(0.0f);
    float rotationSpeed = 1.0f;
    ColorGradient multiplyColorGradient;
    ColorGradient addColorGradient;

    bool addColor = true;
    bool randomVelocity = true;
    bool drawVelocity = false;
    bool randomLifetime = true;
    float startVelocity = 1.0f;

    glm::vec3 acceleration = glm::vec3(0.0f);

    float startLifeTime = 1.0f;
    float minLifetime = 1.0f, maxLifetime = 1.0f;

    template <class Archive>
    void serialize(Archive& archive)
    {
        make_optional_nvp(archive, "startSize", startSize);
        make_optional_nvp(archive, "endSize", endSize);
        make_optional_nvp(archive, "sizeEase", sizeEase);
        make_optional_nvp(archive, "randomStartRotation", randomStartRotation);
        make_optional_nvp(archive, "startRotation", startRotation);
        make_optional_nvp(archive, "randomRotate", randomRotate);
        make_optional_nvp(archive, "rotationSpeed", rotationSpeed);
        make_optional_nvp(archive, "randomColor", randomColor);
        make_optional_nvp(archive, "multiplyColor", multiplyColor);
        make_optional_nvp(archive, "multiplyColorGradient", multiplyColorGradient);
        make_optional_nvp(archive, "addColor", addColor);
        make_optional_nvp(archive, "addColorGradient", addColorGradient);
        make_optional_nvp(archive, "randomVelocity", randomVelocity);
        make_optional_nvp(archive, "drawVelocity", drawVelocity);
        make_optional_nvp(archive, "startVelocity", startVelocity);
        make_optional_nvp(archive, "acceleration", acceleration);
        make_optional_nvp(archive, "randomLifetime", randomLifetime);
        make_optional_nvp(archive, "startLifeTime", startLifeTime);
        make_optional_nvp(archive, "minLifetime", minLifetime);
        make_optional_nvp(archive, "maxLifetime", maxLifetime);
    }
};

struct Emitter
{
    EmitterSpecs specs;
    ParticleSpecs particleSpecs;
    float time = 0.0f;
    int particleCount = 0;
    int id = 0;

    // cerial
    template <class Archive>
    void save(Archive& archive) const
    {
        // archive(cereal::make_nvp("specs", specs), cereal::make_nvp("particleSpecs", particleSpecs));
        make_optional_nvp(archive, "specs", specs);
        make_optional_nvp(archive, "particleSpecs", particleSpecs);
    }

    template <class Archive>
    void load(Archive& archive)
    {
        // archive(cereal::make_nvp("specs", specs), cereal::make_nvp("particleSpecs", particleSpecs));
        make_optional_nvp(archive, "specs", specs);
        make_optional_nvp(archive, "particleSpecs", particleSpecs);
        id = rand();
    }
};
#pragma endregion ParticleSystem

struct Camera
{
    bool render = true;
    float fov, aspectRatio, nearClip, farClip;
    bool orthographic = false;
    float orthoSize = 10.0f;
    RenderMode renderMode = RenderMode::Standard;

    Camera() = default;
    Camera(float fov, float aspectRatio, float nearClip, float farClip);

    glm::mat4 GetProjectionMatrix() const;
    static glm::mat4 GetViewMatrix(glm::vec3 position, glm::quat rotation);
    glm::mat4 GetViewProjectionMatrix(glm::vec3 position, glm::quat rotation) const;

    float GetFov() const { return fov; }
    void SetFov(const float newFov) { this->fov = newFov; }

    float GetAspectRatio() const { return aspectRatio; }
    void SetAspectRatio(const float newAspectRatio) { this->aspectRatio = newAspectRatio; }

    float GetNearClip() const { return nearClip; }
    void SetNearClip(const float newNearClip) { this->nearClip = newNearClip; }

    float GetFarClip() const { return farClip; }
    void SetFarClip(const float newFarClip) { this->farClip = newFarClip; }

    raycasting::Ray ScreenPointToRay(const glm::vec2& screenPoint,
                                     const glm::vec3& position,
                                     const glm::quat& rotation,
                                     const float screenWidth,
                                     const float screenHeight) const;

    glm::vec3 GetAnchorPosition(Anchor type, const glm::vec3& position, const glm::quat& rotation) const;

    float SmallestDepthDifference() const;

    template <class Archive>
    void save(Archive& archive) const
    {
        make_optional_nvp(archive, "fov", fov);
        make_optional_nvp(archive, "aspectRatio", aspectRatio);
        make_optional_nvp(archive, "nearClip", nearClip);
        make_optional_nvp(archive, "farClip", farClip);
        make_optional_nvp(archive, "orthographic", orthographic);
        make_optional_nvp(archive, "orthoSize", orthoSize);
        make_optional_nvp(archive, "renderMode", renderMode);
    }

    template <class Archive>
    void load(Archive& archive)
    {
        make_optional_nvp(archive, "fov", fov);
        make_optional_nvp(archive, "aspectRatio", aspectRatio);
        make_optional_nvp(archive, "nearClip", nearClip);
        make_optional_nvp(archive, "farClip", farClip);
        make_optional_nvp(archive, "orthographic", orthographic);
        make_optional_nvp(archive, "orthoSize", orthoSize);
        make_optional_nvp(archive, "renderMode", renderMode);
    }
};

struct GltfScene
{
    fs::path path;
    std::string name;

    template <class Archive>
    void save(Archive& archive) const
    {
        // archive(cereal::make_nvp("path", path), cereal::make_nvp("name", name));
        make_optional_nvp(archive, "path", path.string());
        make_optional_nvp(archive, "name", name);
    }

    template <class Archive>
    void load(Archive& archive)
    {
        // archive(cereal::make_nvp("path", path), cereal::make_nvp("name", name));
        std::string oldPath = "";
        make_optional_nvp(archive, "path", oldPath);
        path = oldPath;
        make_optional_nvp(archive, "name", name);
    }
};

struct GltfNode
{
    int meshId = -1;
    std::string name;

    template <class Archive>
    void save(Archive& archive) const
    {
        // archive(cereal::make_nvp("meshId", meshId), cereal::make_nvp("name", name));
        make_optional_nvp(archive, "meshId", meshId);
        make_optional_nvp(archive, "name", name);
    }

    template <class Archive>
    void load(Archive& archive)
    {
        // archive(cereal::make_nvp("meshId", meshId), cereal::make_nvp("name", name));
        make_optional_nvp(archive, "meshId", meshId);
        make_optional_nvp(archive, "name", name);
    }
};

struct AssetItem
{
    fs::path path;
    Ref<FrameBuffer> frameBuffer;

    AssetItem() = default;

    AssetItem(const int width, const int height, const fs::path& path) : path(path)
    {
        FrameBufferSettings settings;
        settings.Width = width;
        settings.Height = height;

        frameBuffer = FrameBuffer::Create(settings);
    };

    ~AssetItem()
    {
        bee::Log::Info("Deleting frameBuffer");
        frameBuffer.reset();
    }

    template <class Archive>
    void save(Archive& archive) const
    {
        // archive(cereal::make_nvp("path", path));
        make_optional_nvp(archive, "path", path);
    }

    template <class Archive>
    void load(Archive& archive)
    {
        // archive(cereal::make_nvp("path", path));
        make_optional_nvp(archive, "path", path);
    }
};

struct Grid
{
    int width = 10;
    int height = 10;
    float tileSize = 1.0f;
    float spacing = 0.0f;
    bool showGrid = true;

    std::vector<std::vector<entt::entity>> cells;

    Grid() = default;

    // deconstructor
    ~Grid()
    {
        for (std::vector list : cells)
        {
            for (entt::entity cell : list)
            {
                if (cell != entt::null && bee::Engine.Registry().valid(cell))
                {
                    bee::Engine.Registry().destroy(cell);
                }
            }
        }

        cells.clear();
    }

    template <class Archive>
    void save(Archive& archive) const
    {
        make_optional_nvp(archive, "width", width);
        make_optional_nvp(archive, "height", height);
        make_optional_nvp(archive, "tileSize", tileSize);
        make_optional_nvp(archive, "spacing", spacing);
        make_optional_nvp(archive, "showGrid", showGrid);
    }

    template <class Archive>
    void load(Archive& archive)
    {
        make_optional_nvp(archive, "width", width);
        make_optional_nvp(archive, "height", height);
        make_optional_nvp(archive, "tileSize", tileSize);
        make_optional_nvp(archive, "spacing", spacing);
        make_optional_nvp(archive, "showGrid", showGrid);
    }
};

struct Cell
{
    entt::entity entity = entt::null;
    entt::entity gridParent = entt::null;
    glm::int2 gridPosition = glm::int2(0, 0);

    template <class Archive>
    void save(Archive& archive) const
    {
        make_optional_nvp(archive, "entity", entity);
        make_optional_nvp(archive, "gridParent", gridParent);
        make_optional_nvp(archive, "gridPosition", gridPosition);
    }

    template <class Archive>
    void load(Archive& archive)
    {
        make_optional_nvp(archive, "entity", entity);
        make_optional_nvp(archive, "gridParent", gridParent);
        make_optional_nvp(archive, "gridPosition", gridPosition);
    }
};

struct Canvas
{
    char padding;  // just here for the save/load functions

    template <class Archive>
    void save(Archive& archive) const
    {
        make_optional_nvp(archive, "padding", padding);
    }

    template <class Archive>
    void load(Archive& archive)
    {
        make_optional_nvp(archive, "padding", padding);
    }
};

struct CanvasElement
{
    Anchor anchor = Anchor::Center;

    template <class Archive>
    void save(Archive& archive) const
    {
        make_optional_nvp(archive, "anchor", anchor);
    }

    template <class Archive>
    void load(Archive& archive)
    {
        make_optional_nvp(archive, "anchor", anchor);
    }
};

static inline glm::mat4 GetWorldModel(const entt::entity entity, entt::registry& registry)
{
    // Check for the Transform component
    auto& transform = registry.get<Transform>(entity);

    // Try to get the HierarchyNode component
    if (auto* hierarchy = registry.try_get<HierarchyNode>(entity))
    {
        // Check if the entity has a CanvasElement for UI-specific logic
        if (auto* canvasElement = registry.try_get<CanvasElement>(entity))
        {
            // If the parent has a Camera component, adjust for UI position
            if (auto* camera = registry.try_get<Camera>(hierarchy->parent))
            {
                glm::vec3 uiPosition = camera->GetAnchorPosition(canvasElement->anchor,
                                                                 glm::vec3(0.0f),  // Local position
                                                                 glm::quat());     // Local rotation (identity)

                glm::mat4 parentTransform = GetWorldModel(hierarchy->parent, registry);
                glm::mat4 translated = glm::translate(glm::mat4(1.0f), uiPosition);
                return parentTransform * translated * transform.GetModelMatrix();
            }
        }

        // Get the parent transform recursively if the parent exists
        if (hierarchy->parent != entt::null)
        {
            glm::mat4 parentTransform = GetWorldModel(hierarchy->parent, registry);
            return parentTransform * transform.GetModelMatrix();
        }
    }

    // Return the local model matrix if no parent is found
    return transform.GetModelMatrix();
}

static inline void SetWorldModel(entt::entity entity, const glm::mat4& modelMatrix, entt::registry& registry)
{
    auto& transform = registry.get<Transform>(entity);

    // Check for the HierarchyNode component
    if (auto* hierarchy = registry.try_get<HierarchyNode>(entity))
    {
        // Check if the entity has a CanvasElement for UI-specific logic
        if (auto* canvasElement = registry.try_get<CanvasElement>(entity))
        {
            // If the parent has a Camera component, adjust for UI position
            if (auto* camera = registry.try_get<Camera>(hierarchy->parent))
            {
                glm::vec3 uiPosition = camera->GetAnchorPosition(canvasElement->anchor,
                                                                 glm::vec3(0.0f),  // Local position
                                                                 glm::quat());     // Local rotation (identity)

                // Compute the local position relative to the camera anchor
                glm::mat4 translated = glm::translate(glm::mat4(1.0f), uiPosition);
                glm::mat4 localModelMatrix = glm::inverse(translated) * modelMatrix;

                // Get the parent's world model matrix for proper hierarchy handling
                glm::mat4 parentWorldModel = GetWorldModel(hierarchy->parent, registry);
                localModelMatrix = glm::inverse(parentWorldModel) * localModelMatrix;

                // Set the local model matrix
                transform.SetModelMatrix(localModelMatrix);
                return;
            }
        }

        // Handle standard non-UI entities with a parent
        if (hierarchy->parent != entt::null)
        {
            // Get the parent's world model matrix
            glm::mat4 parentWorldModel = GetWorldModel(hierarchy->parent, registry);

            // Compute the local model matrix using the parent's world model matrix
            glm::mat4 localModelMatrix = glm::inverse(parentWorldModel) * modelMatrix;

            // Set the local model matrix
            transform.SetModelMatrix(localModelMatrix);
            return;
        }
    }

    // If no parent or no special UI handling, set the world model matrix directly
    transform.SetModelMatrix(modelMatrix);
}

static inline glm::vec3 GetWorldPosition(entt::entity entity, entt::registry& registry)
{
    glm::mat4 worldModel = GetWorldModel(entity, registry);
    return glm::vec3(worldModel[3]);
}

static inline void SetWorldPosition(entt::entity entity, glm::vec3& position, entt::registry& registry)
{
    glm::mat4 worldModel = GetWorldModel(entity, registry);

    // Modify the translation part of the model matrix with the new position
    worldModel[3] = glm::vec4(position, 1.0f);

    SetWorldModel(entity, worldModel, registry);
}

}  // namespace bee


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/