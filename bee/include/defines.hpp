#pragma once

#define SCENE_EXTENSION ".scene"
#define PREFAB_EXTENSION ".prefab"
#define EMITTER_EXTENSION ".emitter"

#define DEFAULT_MODEL "models/cube.obj"
#define DEFAULT_TEXTURE "textures/white.png"
#define DEFAULT_QUAD "models/quad.obj"

template <typename T>
using Scope = std::unique_ptr<T>;

template <typename T, typename... Args>
constexpr Scope<T> CreateScope(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T>
using Ref = std::shared_ptr<T>;

template <typename T, typename... Args>
constexpr Ref<T> CreateRef(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

// weak ptr
template <typename T>
using WeakRef = std::weak_ptr<T>;

namespace fs = std::filesystem;

namespace bee
{
enum RenderMode
{
    Standard,
    Normal,
    UV,
    Texture,
    VertexColor,
    Wireframe,
    Count  // needed for ImGui
};

enum class Anchor
{
    TopLeft,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight,
    Count  // needed for ImGui
};
}  // namespace bee


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/