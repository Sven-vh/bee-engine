#include "managers/render_manager.hpp"
#include "core.hpp"

using namespace bee;

std::unordered_map<entt::entity, glm::mat4> RenderManager::m_worldModelCache;
std::unordered_map<RenderMode, xsr::shader_handle> RenderManager::m_shaders;

void bee::RenderManager::Initialize()
{
    m_shaders[RenderMode::Standard] = CreateDefaultShader();
    m_shaders[RenderMode::Normal] = CreateNormalShader();
    m_shaders[RenderMode::UV] = CreateUVShader();
    m_shaders[RenderMode::Texture] = CreateTextureShader();
    m_shaders[RenderMode::VertexColor] = CreateVertexColorShader();
    m_shaders[RenderMode::Wireframe] = m_shaders[RenderMode::Standard];

    xsr::set_standard_shader(m_shaders[RenderMode::Standard]);
}

void bee::RenderManager::SubmitRenderables(entt::registry& registry)
{
    PROFILE_FUNCTION();
    const auto view = bee::ecs::GetView<Renderable, Transform>(registry /*, entt::type_list<CanvasElement>()*/);
    for (const auto entity : view)
    {
        const auto& renderable = view.get<Renderable>(entity);

        if (!renderable.visible) continue;
        if (renderable.billboard) continue;

        glm::mat4 model = GetWorldModel(entity, registry);

        bool succes = xsr::render_mesh(glm::value_ptr(model),
                                       renderable.mesh->GetHandle(),
                                       renderable.texture->GetHandle(),
                                       glm::value_ptr(renderable.multiplier),
                                       glm::value_ptr(renderable.tint),
                                       renderable.receiveShadows);
        if (!succes)
        {
            bee::Log::Error("Failed to render mesh");
        }
    }
}

void bee::RenderManager::SubmitUIRenderables(entt::registry& registry)
{
    PROFILE_FUNCTION();
    const auto view = bee::ecs::GetView<Renderable, Transform, CanvasElement>(registry);
    for (const auto entity : view)
    {
        const auto& renderable = view.get<Renderable>(entity);

        if (!renderable.visible) continue;

        glm::mat4 model = GetWorldModel(entity, registry);

        bool succes = xsr::render_mesh(glm::value_ptr(model),
                                       renderable.mesh->GetHandle(),
                                       renderable.texture->GetHandle(),
                                       glm::value_ptr(renderable.multiplier),
                                       glm::value_ptr(renderable.tint),
                                       renderable.receiveShadows);
        if (!succes)
        {
            bee::Log::Error("Failed to render mesh");
        }
    }
}

void bee::RenderManager::SubmitBillboards(const entt::entity cameraEntity, entt::registry& registry)
{
    const glm::mat4 cameraModel = GetWorldModel(cameraEntity, registry);
    const glm::vec3 cameraPosition = cameraModel[3];

    const auto view = registry.view<Renderable, Transform>();
    for (const auto entity : view)
    {
        const auto& renderable = view.get<Renderable>(entity);
        if (!renderable.visible) continue;
        if (!renderable.billboard) continue;

        glm::mat4 model = GetWorldModelCache(entity, registry);

        model = bee::helper::LookToPosition(model, cameraPosition, false);

        bool succes = xsr::render_mesh(glm::value_ptr(model),
                                       renderable.mesh->GetHandle(),
                                       renderable.texture->GetHandle(),
                                       glm::value_ptr(renderable.multiplier),
                                       glm::value_ptr(renderable.tint),
                                       renderable.receiveShadows);
        if (!succes)
        {
            bee::Log::Error("Failed to render mesh");
        }
    }
}

void bee::RenderManager::SubmitLights(entt::registry& registry)
{
    const auto lightView = registry.view<DirectionalLight, Transform>();

    for (auto entity : lightView)
    {
        auto& light = lightView.get<DirectionalLight>(entity);
        auto& transform = lightView.get<Transform>(entity);
        xsr::render_directional_light(glm::value_ptr(transform.GetDirection()), glm::value_ptr(light.color * light.color.a));

        // Debug line for arrow direction
        xsr::render_debug_line(glm::value_ptr(transform.GetPosition()),
                               glm::value_ptr(transform.GetPosition() + transform.GetDirection()),
                               glm::value_ptr(light.color));
    }

    auto pointLightView = registry.view<PointLight, Transform>();
    for (auto entity : pointLightView)
    {
        auto& light = pointLightView.get<PointLight>(entity);
        auto& transform = pointLightView.get<Transform>(entity);
        xsr::render_point_light(glm::value_ptr(transform.GetPosition()), light.range, glm::value_ptr(light.color));
    }
}

void bee::RenderManager::SubmitSceneData(entt::registry& registry)
{
    auto sceneDataView = registry.view<SceneData>();

    if (sceneDataView.size() > 1)
    {
        bee::Log::Warn("More than one SceneData component found in the scene. Using the first one.");
    }

    if (!sceneDataView.empty())
    {
        auto& sceneData = sceneDataView.get<SceneData>(sceneDataView.front());
        xsr::render_ambient_light(glm::value_ptr(sceneData.ambientColor));
        xsr::set_background_gradient(sceneData.skyColor);
    }
}

void bee::RenderManager::ClearEntries()
{
    xsr::clear_entries();
    m_worldModelCache.clear();
}

void bee::RenderManager::Render(const entt::entity cameraEntity, entt::registry& registry)
{
    PROFILE_FUNCTION();
    const bee::Camera& camera = registry.get<bee::Camera>(cameraEntity);

    const glm::mat4 cameraModel = GetWorldModel(cameraEntity, registry);
    const glm::vec3 cameraPosition = cameraModel[3];
    const glm::quat cameraRotation = glm::quat_cast(cameraModel);

    const float* view = value_ptr(camera.GetViewMatrix(cameraPosition, cameraRotation));
    const float* projection = value_ptr(camera.GetProjectionMatrix());

    xsr::enable_depth_test(true);

    xsr::clear_screen();
    xsr::render_background(view, projection);
#ifdef EDITOR_MODE
    xsr::render_grid(view, projection);
#endif

    xsr::shader_handle shader = m_shaders[camera.renderMode];

    if (camera.renderMode == RenderMode::Wireframe)
    {
        xsr::enable_wireframes(true);
    }
    else
    {
        xsr::enable_wireframes(false);
    }

    xsr::render(view, projection, shader);

    xsr::enable_wireframes(false);
}

void bee::RenderManager::RenderUI(const entt::entity cameraEntity, entt::registry& registry)
{
    PROFILE_FUNCTION();
    const bee::Camera& camera = registry.get<bee::Camera>(cameraEntity);

    const glm::mat4 cameraModel = GetWorldModel(cameraEntity, registry);
    const glm::vec3 cameraPosition = cameraModel[3];
    const glm::quat cameraRotation = glm::quat_cast(cameraModel);

    xsr::shader_handle shader = m_shaders[camera.renderMode];

    xsr::enable_depth_test(false);

    if (camera.renderMode == RenderMode::Wireframe)
    {
        xsr::enable_wireframes(true);
    }
    else
    {
        xsr::enable_wireframes(false);
    }

    xsr::render(value_ptr(camera.GetViewMatrix(cameraPosition, cameraRotation)),
                value_ptr(camera.GetProjectionMatrix()),
                shader);
}

const glm::mat4& bee::RenderManager::GetWorldModelCache(const entt::entity entity, entt::registry& registry)
{
    auto& transform = registry.get<Transform>(entity);

    // try to get parent transform
    if (registry.all_of<HierarchyNode>(entity))
    {
        auto& hierarchy = registry.get<HierarchyNode>(entity);
        if (hierarchy.parent != entt::null && entity != hierarchy.parent)
        {
            glm::mat4 parentTransform = GetWorldModelCache(hierarchy.parent, registry);
            m_worldModelCache[entity] = parentTransform * transform.GetModelMatrix();
            return m_worldModelCache[entity];
        }
    }

    m_worldModelCache[entity] = transform.GetModelMatrix();
    return m_worldModelCache[entity];
}

xsr::shader_handle bee::RenderManager::CreateDefaultShader()
{
    const char* vs_str = R"(
#version 460 core

// Vertex attributes
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texture_coordinate;
layout(location = 3) in vec3 a_color;

// Instanced attributes
layout(location = 4) in mat4 a_model;
//location 7 got removed
layout(location = 8) in vec4 a_mul_color;
layout(location = 9) in vec4 a_add_color;
layout(location = 10) in float a_receive_shadows;

// Outputs to fragment shader
out vec3 v_normal;
out vec2 v_texture_coordinate;
out vec4 v_color;
out vec4 v_mul_color;     // Added
out vec4 v_add_color;     // Added
flat out float v_receive_shadows;

uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    // Transform the normal with the model matrix
    v_normal = normalize(mat3(a_model) * a_normal);

    // Pass through texture coordinates and color
    v_texture_coordinate = a_texture_coordinate;
    v_color = vec4(a_color, 1.0);

    // Pass per-instance data to the fragment shader
    v_mul_color = a_mul_color;    // Added
    v_add_color = a_add_color;    // Added
    v_receive_shadows = a_receive_shadows;

    // Calculate the vertex position
    gl_Position = u_projection * u_view * a_model * vec4(a_position, 1.0);
}
)";
    // I used ChatGPT to help me with the shaders for transparency
    const char* fs_str = R"(
#version 460 core

in vec4 v_color;
in vec2 v_texture_coordinate;
in vec3 v_normal;
in vec4 v_mul_color;
in vec4 v_add_color;
flat in float v_receive_shadows;

out vec4 frag_color;

uniform sampler2D u_texture;
uniform int u_num_directional_lights;
uniform vec4 u_ambient_light;

const int MAX_DIR_LIGHTS = 4;

struct directional_light
{
    vec3 direction;
    vec3 color;
};

uniform directional_light u_directional_lights[MAX_DIR_LIGHTS];

void main()
{
    // Sample the texture color
    vec4 texture_color = texture(u_texture, v_texture_coordinate);

    // Initialize total light with ambient light, considering its alpha
    vec3 total_light = u_ambient_light.rgb * u_ambient_light.a;

    if (v_receive_shadows > 0.5)
    {
        vec3 normal = normalize(v_normal);

        // Calculate lighting from directional lights
        for (int i = 0; i < u_num_directional_lights; i++)
        {
            directional_light light = u_directional_lights[i];
            vec3 light_dir = normalize(-light.direction);
            float intensity = max(dot(normal, light_dir), 0.0);
            total_light += light.color * intensity;
        }
    }
    else
    {
        // Ensure full brightness if not receiving shadows
        total_light += vec3(1.0);
    }

    // Clamp total light to prevent overexposure
    total_light = clamp(total_light, 0.0, 1.0);

    // Apply lighting to the texture color
    vec3 lit_color_rgb = texture_color.rgb * total_light;

    // Create a local copy of mul_color and adjust alpha if necessary
    vec4 mul_color = v_mul_color;
    if (mul_color.a > 0.95)
    {
        mul_color.a = 1.0;
    }

    // Apply multiplication color
    lit_color_rgb *= mul_color.rgb;

    // Apply addition color with clamping to prevent overexposure
    lit_color_rgb = clamp(lit_color_rgb + v_add_color.rgb * v_add_color.a, 0.0, 1.0);

    // Calculate the final alpha value
    float final_alpha = texture_color.a * mul_color.a;

    // Determine the blending factor based on texture alpha
    float t = texture_color.a;

    // Blend between vertex color and lit color based on the blending factor
    vec3 blended_color = mix(v_color.rgb * mul_color.rgb, lit_color_rgb, t);

    // Set the final fragment color
    frag_color = vec4(blended_color, final_alpha);
}
)";

    xsr::shader_handle shader = xsr::create_shader(vs_str, fs_str);
    if (!shader.is_valid())
    {
        bee::Log::Error("Failed to create default shader");
        return xsr::shader_handle();
    }
    return shader;
}

xsr::shader_handle bee::RenderManager::CreateNormalShader()
{
    const char* vs_str = R"(
#version 460 core

// Vertex attributes
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texture_coordinate;
layout(location = 3) in vec3 a_color;

// Instanced attributes
layout(location = 4) in mat4 a_model;
//location 7 got removed
layout(location = 8) in vec4 a_mul_color;
layout(location = 9) in vec4 a_add_color;
layout(location = 10) in float a_receive_shadows;

// Outputs to fragment shader
out vec3 v_normal;
out vec2 v_texture_coordinate;
out vec4 v_color;
out vec4 v_mul_color;     // Added
out vec4 v_add_color;     // Added
flat out float v_receive_shadows;

uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    // Transform the normal with the model matrix
    v_normal = normalize(mat3(a_model) * a_normal);

    // Pass through texture coordinates and color
    v_texture_coordinate = a_texture_coordinate;
    v_color = vec4(a_color, 1.0);

    // Pass per-instance data to the fragment shader
    v_mul_color = a_mul_color;    // Added
    v_add_color = a_add_color;    // Added
    v_receive_shadows = a_receive_shadows;

    // Calculate the vertex position
    gl_Position = u_projection * u_view * a_model * vec4(a_position, 1.0);
}
)";
    // I used ChatGPT to help me with the shaders for transparency
    const char* fs_str = R"(
#version 460 core

in vec4 v_color;
in vec2 v_texture_coordinate;
in vec3 v_normal;
in vec4 v_mul_color;
in vec4 v_add_color;
flat in float v_receive_shadows;

out vec4 frag_color;

uniform sampler2D u_texture;
uniform int u_num_directional_lights;
uniform vec4 u_ambient_light;

const int MAX_DIR_LIGHTS = 4;

struct directional_light
{
    vec3 direction;
    vec3 color;
};

uniform directional_light u_directional_lights[MAX_DIR_LIGHTS];

void main()
{
	// 0.5*color(N.x()+1, N.y()+1, N.z()+1);
    frag_color = vec4(0.5 * (v_normal.x + 1), 0.5 * (v_normal.y + 1), 0.5 * (v_normal.z + 1), 1.0);
}
)";

    xsr::shader_handle shader = xsr::create_shader(vs_str, fs_str);
    if (!shader.is_valid())
    {
        bee::Log::Error("Failed to create normal shader");
        return xsr::shader_handle();
    }
    return shader;
}

xsr::shader_handle bee::RenderManager::CreateUVShader()
{
    const char* vs_str = R"(
#version 460 core

// Vertex attributes
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texture_coordinate;
layout(location = 3) in vec3 a_color;

// Instanced attributes
layout(location = 4) in mat4 a_model;
//location 7 got removed
layout(location = 8) in vec4 a_mul_color;
layout(location = 9) in vec4 a_add_color;
layout(location = 10) in float a_receive_shadows;

// Outputs to fragment shader
out vec2 v_texture_coordinate;
out vec4 v_color;
out vec4 v_mul_color;     // Added
out vec4 v_add_color;     // Added
flat out float v_receive_shadows;

uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    // Pass through texture coordinates and color
    v_texture_coordinate = a_texture_coordinate;
    v_color = vec4(a_color, 1.0);

    // Pass per-instance data to the fragment shader
    v_mul_color = a_mul_color;    // Added
    v_add_color = a_add_color;    // Added
    v_receive_shadows = a_receive_shadows;

    // Calculate the vertex position
    gl_Position = u_projection * u_view * a_model * vec4(a_position, 1.0);
}
)";

    const char* fs_str = R"(
#version 460 core

in vec4 v_color;
in vec2 v_texture_coordinate;
in vec4 v_mul_color;
in vec4 v_add_color;
flat in float v_receive_shadows;

out vec4 frag_color;

uniform sampler2D u_texture;

void main()
{
    // Output UV coordinates as colors (for visualization)
    frag_color = vec4(v_texture_coordinate, 0.0, 1.0);
}
)";

    xsr::shader_handle shader = xsr::create_shader(vs_str, fs_str);
    if (!shader.is_valid())
    {
        bee::Log::Error("Failed to create UV shader");
        return xsr::shader_handle();
    }
    return shader;
}

xsr::shader_handle bee::RenderManager::CreateTextureShader()
{
    const char* vs_str = R"(
#version 460 core

// Vertex attributes
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texture_coordinate;
layout(location = 3) in vec3 a_color;

// Instanced attributes
layout(location = 4) in mat4 a_model;
//location 7 got removed
layout(location = 8) in vec4 a_mul_color;
layout(location = 9) in vec4 a_add_color;
layout(location = 10) in float a_receive_shadows;

// Outputs to fragment shader
out vec2 v_texture_coordinate;
out vec4 v_color;
out vec4 v_mul_color;     // Added
out vec4 v_add_color;     // Added
flat out float v_receive_shadows;

uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    // Pass through texture coordinates and color
    v_texture_coordinate = a_texture_coordinate;
    v_color = vec4(a_color, 1.0);

    // Pass per-instance data to the fragment shader
    v_mul_color = a_mul_color;    // Added
    v_add_color = a_add_color;    // Added
    v_receive_shadows = a_receive_shadows;

    // Calculate the vertex position
    gl_Position = u_projection * u_view * a_model * vec4(a_position, 1.0);
}
)";

    const char* fs_str = R"(
#version 460 core

in vec4 v_color;
in vec2 v_texture_coordinate;
in vec4 v_mul_color;
in vec4 v_add_color;
flat in float v_receive_shadows;

out vec4 frag_color;

uniform sampler2D u_texture;

void main()
{
    frag_color = texture(u_texture, v_texture_coordinate);
}
)";

    xsr::shader_handle shader = xsr::create_shader(vs_str, fs_str);
    if (!shader.is_valid())
    {
        bee::Log::Error("Failed to create UV shader");
        return xsr::shader_handle();
    }
    return shader;
}

xsr::shader_handle bee::RenderManager::CreateVertexColorShader()
{
    const char* vs_str = R"(
#version 460 core

// Vertex attributes
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texture_coordinate;
layout(location = 3) in vec3 a_color;

// Instanced attributes
layout(location = 4) in mat4 a_model;
//location 7 got removed
layout(location = 8) in vec4 a_mul_color;
layout(location = 9) in vec4 a_add_color;
layout(location = 10) in float a_receive_shadows;

// Outputs to fragment shader
out vec2 v_texture_coordinate;
out vec4 v_color;
out vec4 v_mul_color;     // Added
out vec4 v_add_color;     // Added
flat out float v_receive_shadows;

uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    // Pass through texture coordinates and color
    v_texture_coordinate = a_texture_coordinate;
    v_color = vec4(a_color, 1.0);

    // Pass per-instance data to the fragment shader
    v_mul_color = a_mul_color;    // Added
    v_add_color = a_add_color;    // Added
    v_receive_shadows = a_receive_shadows;

    // Calculate the vertex position
    gl_Position = u_projection * u_view * a_model * vec4(a_position, 1.0);
}
)";

    const char* fs_str = R"(
#version 460 core

in vec4 v_color;
in vec2 v_texture_coordinate;
in vec4 v_mul_color;
in vec4 v_add_color;
flat in float v_receive_shadows;

out vec4 frag_color;

uniform sampler2D u_texture;

void main()
{
    frag_color = v_color;
}
)";

    xsr::shader_handle shader = xsr::create_shader(vs_str, fs_str);
    if (!shader.is_valid())
    {
        bee::Log::Error("Failed to create UV shader");
        return xsr::shader_handle();
    }
    return shader;
}



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/