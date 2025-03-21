#include <fstream>
#include <sstream>
#include <array>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <algorithm>
#include <algorithm>

#if defined(_WIN32)
#define NOMINMAX
#ifdef APIENTRY
#undef APIENTRY
#endif
#include <windows.h>
#endif

#include "xsr.hpp"

#define STRINGIFY(x) #x
#define DEBUG_LINES 1

using namespace xsr;
using namespace glm;

#define XSR_USE_HARD_CODED_SHADERS

namespace std
{
template <>
struct hash<mesh_handle>
{
    std::size_t operator()(const mesh_handle& handle) const noexcept
    {
        return std::hash<int>()(handle.id);  // Hash the 'id' member
    }
};

template <>
struct hash<texture_handle>
{
    std::size_t operator()(const texture_handle& handle) const noexcept
    {
        return std::hash<int>()(handle.id);  // Hash the 'id' member
    }
};

template <>
struct hash<std::pair<mesh_handle, texture_handle>>
{
    std::size_t operator()(const std::pair<mesh_handle, texture_handle>& pair) const noexcept
    {
        return std::hash<int>()(pair.first.id) ^ std::hash<int>()(pair.second.id);
    }
};

}  // namespace std

namespace xsr::internal
{
GLFWwindow* window = nullptr;
render_configuration rconfig;
device_configuration dconfig;

shader_handle compile_standard_shader();
shader_handle compile_normal_shader();
struct Shader;
Shader get_program(const shader_handle& handle);
bool compile_background_shader();
bool compile_grid_shader();
void create_grid_mesh();
void create_fullscreen_quad();
bool compile_shader(GLuint* shader, GLenum type, const GLchar* source);
bool link_program(GLuint program);
void glfw_error_callback(int error, const char* description);
void init_debug_messages();
void init_debug_variables();

#ifdef EDITOR_MODE
static void debug_callback_func(GLenum source,
                                GLenum type,
                                GLuint id,
                                GLenum severity,
                                GLsizei length,
                                const GLchar* message,
                                const GLvoid* userParam);

#if defined(_WIN32)
void debug_callback_func_amd(GLuint id,
                             GLenum category,
                             GLenum severity,
                             GLsizei length,
                             const GLchar* message,
                             void* userParam);
#endif
#endif

struct Mesh
{
    unsigned int vao = 0;
    unsigned int ebo = 0;
    std::array<unsigned int, 4> vbos;
    unsigned int instanceVBO = 0;
    uint32_t count = 0;
};

struct Texture
{
    unsigned int id = 0;
    int width = 0;
    int height = 0;
    int channels = 0;
};

struct DirectionalLight
{
    vec3 direction;
    vec3 color;
};

struct PointLight
{
    vec3 position;
    float range;
    vec3 color;
};

struct AmbientLight
{
    vec4 color;
};

struct Entry
{
    mat4 transform;
    mesh_handle mesh;
    texture_handle texture;
    vec4 mul_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    vec4 add_color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    bool receive_shadows = true;
};

struct Shader
{
    unsigned int program = 0;

    operator unsigned int() const { return program; }

    // assign operator
    Shader& operator=(unsigned int value)
    {
        program = value;
        return *this;
    }
};

// Interleave per-instance data into a single buffer
struct InstanceData
{
    glm::mat4 model;
    glm::vec4 mul_color;
    glm::vec4 add_color;
    float receive_shadows;
    float padding[3];  // Padding to make the struct size a multiple of 16 bytes

    InstanceData(const glm::mat4& model, const glm::vec4& mul_color, const glm::vec4& add_color, float receive_shadows)
        : model(model), mul_color(mul_color), add_color(add_color), receive_shadows(receive_shadows)
    {
    }
};

using Instance = std::pair<mesh_handle, texture_handle>;
std::unordered_map<Instance, std::vector<InstanceData>> instances;
std::unordered_map<Instance, std::vector<InstanceData>> transparentInstances;

// All the meshes
std::vector<Mesh> meshes;

mesh_handle default_mesh;
mesh_handle default_quad_mesh;

// All the textures
std::vector<Texture> textures;

std::vector<Shader> shaders;

// The standard program
Shader standard_program;
// unsigned int standard_program = 0;
//  unsigned int debug_program = 0;

// All the active objects
std::vector<DirectionalLight> dir_lights;
std::vector<PointLight> point_lights;
AmbientLight ambient_light;
std::vector<Entry> entries;

#ifdef EDITOR_MODE
static int const m_maxLines = 16380;
int m_linesCount = 0;
struct VertexPosition3DColor
{
    glm::vec3 Position;
    glm::vec4 Color;
};
VertexPosition3DColor* m_vertexArray = nullptr;
unsigned int debug_program = 0;
unsigned int m_linesVAO = 0;
unsigned int m_linesVBO = 0;
#endif

GLuint background_program = 0;
GLuint background_vao = 0;
bee::ColorGradient background_gradient;
bool background_gradient_set = false;

GLuint grid_program = 0;
GLuint grid_vao = 0;
GLuint grid_vbo = 0;
GLuint grid_ebo = 0;
GLuint grid_index_count = 0;

grid_settings m_grid_settings;

int drawCalls = 0;

void DrawInstancedGroup(const std::vector<InstanceData>& group,
                        const mesh_handle mesh_handle,
                        const texture_handle texture_handle,
                        const GLint texture_location);

unsigned int use_shader(const xsr::shader_handle& shader);

}  // namespace xsr::internal

using namespace xsr::internal;

grid_settings& xsr::get_grid_settings() { return internal::m_grid_settings; }

bool xsr::initialize(const render_configuration& config)
{
    internal::rconfig = config;

    // TODO: Use the config
    // TODO: Handle errors and return false if something fails

    internal::init_debug_messages();
    internal::init_debug_variables();
    internal::compile_background_shader();
    internal::create_fullscreen_quad();

    // Compile the grid shader
    internal::compile_grid_shader();
    internal::create_grid_mesh();

    return true;
}

void xsr::shutdown()
{
    // Delete all the meshes
    for (auto& mesh : internal::meshes)
    {
        glDeleteVertexArrays(1, &mesh.vao);
        glDeleteBuffers(1, &mesh.ebo);
        glDeleteBuffers((GLsizei)mesh.vbos.size(), mesh.vbos.data());
    }

    // Delete all the textures
    for (auto& texture : internal::textures) glDeleteTextures(1, &texture.id);

    // Delete the standard shader
    glDeleteProgram(internal::standard_program);
}

mesh_handle xsr::create_mesh(const unsigned int* indices,
                             unsigned int index_count,
                             const float* positions,
                             const float* normals,
                             const float* texture_coordinates,
                             const float* colors,
                             unsigned int vertex_count)
{
    // Create an OpenGL mesh
    internal::Mesh mesh;
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    // Create the index buffer
    glGenBuffers(1, &mesh.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(int), indices, GL_STATIC_DRAW);
    mesh.count = index_count;

    // Create the vertex buffers
    glGenBuffers((GLsizei)mesh.vbos.size(), mesh.vbos.data());
    // glGenBuffers(1, mesh.vbos.data());

    // Create the position buffer
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), positions, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Create the normal buffer
    if (normals)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[1]);
        glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), normals, GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    // Create the texture coordinate buffer
    if (texture_coordinates)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[2]);
        glBufferData(GL_ARRAY_BUFFER, vertex_count * 2 * sizeof(float), texture_coordinates, GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    // Create the color buffer
    if (colors)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[3]);
        glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), colors, GL_STATIC_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    }
    else
    {
        // Create a white color buffer
        std::vector<float> white_colors(vertex_count * 3, 1.0f);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[3]);
        glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), white_colors.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    // Unbind the vertex array
    glBindVertexArray(0);

    // loop through all the positions and find the min and max values
    // this will be used to calculate the bounding box of the mesh
    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float min_z = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::min();
    float max_y = std::numeric_limits<float>::min();
    float max_z = std::numeric_limits<float>::min();

    for (unsigned int i = 0; i < vertex_count; ++i)
    {
        min_x = std::min(min_x, positions[i * 3 + 0]);
        min_y = std::min(min_y, positions[i * 3 + 1]);
        min_z = std::min(min_z, positions[i * 3 + 2]);
        max_x = std::max(max_x, positions[i * 3 + 0]);
        max_y = std::max(max_y, positions[i * 3 + 1]);
        max_z = std::max(max_z, positions[i * 3 + 2]);
    }

    // Calculate the bounding box
    vec3 min = vec3(min_x, min_y, min_z);
    vec3 max = vec3(max_x, max_y, max_z);
    vec3 size = max - min;
    vec3 center = (min + max) / 2.0f;

    // Store the mesh
    internal::meshes.push_back(mesh);
    mesh_handle handle{(int)internal::meshes.size()};
    handle.meshSize = size;
    handle.meshCenter = center;
    return handle;
}

void xsr::unload_mesh(mesh_handle mesh)
{
    if (mesh.id <= 0 || mesh.id > (int)internal::meshes.size()) return;
    // Delete the mesh
    glDeleteVertexArrays(1, &internal::meshes[mesh.id - 1].vao);
    glDeleteBuffers(1, &internal::meshes[mesh.id - 1].ebo);
    glDeleteBuffers((GLsizei)internal::meshes[mesh.id - 1].vbos.size(), internal::meshes[mesh.id - 1].vbos.data());

    // set the mesh handle to invalid
    internal::meshes[mesh.id - 1].vao = 0;
}

texture_handle xsr::create_texture(int width, int height, const void* pixel_rgba_bytes)
{
    // Create an OpenGL texture
    internal::Texture texture;
    glGenTextures(1, &texture.id);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel_rgba_bytes);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Store the texture
    internal::textures.push_back(texture);
    return texture_handle{(int)internal::textures.size()};
}

void xsr::unload_texture(texture_handle texture)
{
    if (texture.id <= 0 || texture.id > (int)internal::textures.size()) return;
    // Delete the texture
    glDeleteTextures(1, &internal::textures[texture.id - 1].id);

    // set the mesh handle to invalid
    internal::textures[texture.id - 1].id = 0;
}

void* xsr::get_texture_id(texture_handle texture)
{
    if (texture.is_valid())
    {
        return (void*)(intptr_t)(internal::textures[texture.id - 1].id);
    }
    return nullptr;
}

void xsr::set_background_gradient(const bee::ColorGradient& gradient)
{
    internal::background_gradient = gradient;
    internal::background_gradient_set = true;
}

void xsr::enable_wireframes(bool enable)
{
    if (enable)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

bool xsr::render_directional_light(const float* direction, const float* color_and_intensity)
{
    if (internal::dir_lights.size() >= 4) return false;

    // Add to the list of active directional lights
    internal::dir_lights.push_back(
        {{direction[0], direction[1], direction[2]}, {color_and_intensity[0], color_and_intensity[1], color_and_intensity[2]}});

    return true;
}

bool xsr::render_point_light(const float* position, float radius, const float* color_and_intensity)
{
    if (internal::point_lights.size() >= 4) return false;

    // Add to the list of active point lights
    internal::point_lights.push_back({{position[0], position[1], position[2]},
                                      radius,
                                      {color_and_intensity[0], color_and_intensity[1], color_and_intensity[2]}});

    return true;
}

bool xsr::render_ambient_light(const float* color_and_intensity)
{
    ambient_light = {{color_and_intensity[0], color_and_intensity[1], color_and_intensity[2], color_and_intensity[3]}};
    return true;
}

bool xsr::render_mesh(const float* transform,
                      const mesh_handle mesh,
                      const texture_handle texture,
                      const float* mul_color,
                      const float* add_color,
                      bool receive_shadows)
{
    // Check that the mesh handle is valid
    if (mesh.id <= 0 || mesh.id > (int)meshes.size()) return false;

    // Check that the texture handle is valid
    if (texture.id <= 0 || texture.id > (int)textures.size()) return false;

    glm::mat4 model = glm::make_mat4(transform);
    glm::vec4 mul = glm::make_vec4(mul_color);
    glm::vec4 add = glm::make_vec4(add_color);
    float shadows = receive_shadows ? 1.0f : 0.0f;

    auto& vec = (mul.a == 1.0f) ? instances.try_emplace({mesh, texture}).first->second
                                : transparentInstances.try_emplace({mesh, texture}).first->second;
    vec.emplace_back(model, mul, add, shadows);

    return true;
}

#ifdef EDITOR_MODE
bool xsr::render_debug_line(const float* from, const float* to, const float* color)
{
    if (m_linesCount < m_maxLines)
    {
        m_vertexArray[m_linesCount * 2].Position = make_vec3(from);
        m_vertexArray[m_linesCount * 2 + 1].Position = make_vec3(to);
        m_vertexArray[m_linesCount * 2].Color = make_vec4(color);
        m_vertexArray[m_linesCount * 2 + 1].Color = make_vec4(color);
        ++m_linesCount;
        return true;
    }
    return false;
}

bool xsr::render_debug_text(const std::string&, const float*, const float*, float) { return false; }
#else

// ignore warning C4100
#pragma warning(push)
#pragma warning(disable : 4100)
bool xsr::render_debug_line(const float* from, const float* to, const float* color) { return true; }

bool xsr::render_debug_text(const std::string& text, const float* position, const float* color, float size) { return true; }

bool xsr::render_debug_cone(const float* position, const float* direction, float angle, const float* color) { return true; }
#pragma warning(pop)

#endif

//  Helper function to render an individual entry
void render_entry(const internal::Entry& entry,
                  GLint texture_location,
                  const float* /*view*/,
                  const float* /*projection*/,
                  GLuint program)
{
    const auto& mesh = internal::meshes[entry.mesh.id - 1];
    const auto& texture = internal::textures[entry.texture.id - 1];
    const auto& transform = entry.transform;

    // Bind the vertex array
    glBindVertexArray(mesh.vao);

    // Bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glUniform1i(texture_location, 0);

    // Send the model matrix to the standard program
    glUniformMatrix4fv(glGetUniformLocation(program, "u_model"), 1, GL_FALSE, value_ptr(transform));

    // Send the multiply and add colors to the standard program
    glUniform4fv(glGetUniformLocation(program, "u_mul_color"), 1, value_ptr(entry.mul_color));
    glUniform4fv(glGetUniformLocation(program, "u_add_color"), 1, value_ptr(entry.add_color));
    glUniform1i(glGetUniformLocation(program, "u_receive_shadows"), entry.receive_shadows);

    // Draw the mesh
    glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, 0);
    drawCalls++;
}

// Most of the instancing code has been written by ChatGPT
// Edit, I had to change it a lot
void xsr::render(const float* view, const float* projection, const shader_handle& shader)
{
    // Clear the screen
    drawCalls = 0;

    // Activate the standard program
    unsigned int program = use_shader(shader);

    //  Send the view and projection matrices to the standard program
    glUniformMatrix4fv(glGetUniformLocation(program, "u_view"), 1, GL_FALSE, view);
    glUniformMatrix4fv(glGetUniformLocation(program, "u_projection"), 1, GL_FALSE, projection);
    auto texture_location = glGetUniformLocation(program, "u_texture");

    // Send the directional lights to the standard program
    glUniform1i(glGetUniformLocation(program, "u_num_directional_lights"), (int)internal::dir_lights.size());
    for (int i = 0; i < internal::dir_lights.size(); ++i)
    {
        std::string name = "u_directional_lights[" + std::to_string(i) + "].direction";
        glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, &internal::dir_lights[i].direction.x);
        name = "u_directional_lights[" + std::to_string(i) + "].color";
        glUniform3fv(glGetUniformLocation(program, name.c_str()), 1, &internal::dir_lights[i].color.x);
    }

    // Send the point lights to the standard program
    glUniform1i(glGetUniformLocation(program, "u_num_point_lights"), (int)internal::point_lights.size());
    for (int i = 0; i < internal::point_lights.size(); ++i)
    {
        std::string name = "u_point_lights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(program, (name + ".m_position").c_str()), 1, &internal::point_lights[i].position.x);
        glUniform1f(glGetUniformLocation(program, (name + ".radius").c_str()), internal::point_lights[i].range);
        glUniform3fv(glGetUniformLocation(program, (name + ".color").c_str()), 1, &internal::point_lights[i].color.x);
    }

    glUniform4fv(glGetUniformLocation(program, "u_ambient_light"), 1, &internal::ambient_light.color.x);

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    for (const auto& group : instances)
    {
        mesh_handle meshHandle = group.first.first;
        texture_handle textureHandle = group.first.second;
        DrawInstancedGroup(group.second, meshHandle, textureHandle, texture_location);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (const auto& group : transparentInstances)
    {
        mesh_handle meshHandle = group.first.first;
        texture_handle textureHandle = group.first.second;
        DrawInstancedGroup(group.second, meshHandle, textureHandle, texture_location);
    }

    // Reset state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

#ifdef EDITOR_MODE
    // Render debug lines
    glm::mat4 vp = make_mat4(projection) * make_mat4(view);
    glUseProgram(debug_program);
    glUniformMatrix4fv(1, 1, false, value_ptr(vp));
    glBindVertexArray(m_linesVAO);

    if (m_linesCount > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_linesVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPosition3DColor) * (m_maxLines * 2), &m_vertexArray[0], GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINES, 0, m_linesCount * 2);
        drawCalls++;
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
#endif

    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        bee::Log::Error("OpenGL Error: {}", error);
    }
}

void xsr::render_background(const float* view, const float* projection, const shader_handle&)
{
    glm::mat4 view_mat = glm::make_mat4(view);
    glm::mat4 proj_mat = glm::make_mat4(projection);
    glm::mat4 inv_view_proj = glm::inverse(proj_mat * view_mat);

    // TODO, make the background an actual shader instead of just a program

    // Render background gradient if set
    if (internal::background_gradient_set)
    {
        // Disable depth test
        enable_depth_test(false);

        // Use background shader
        glUseProgram(internal::background_program);

        // Set inverse view-projection matrix
        glUniformMatrix4fv(glGetUniformLocation(internal::background_program, "u_inv_view_proj"),
                           1,
                           GL_FALSE,
                           glm::value_ptr(inv_view_proj));

        glm::vec3 camera_pos = glm::inverse(view_mat)[3];

        // Pass camera position to the shader
        glUniform3fv(glGetUniformLocation(internal::background_program, "u_camera_pos"), 1, glm::value_ptr(camera_pos));

        // Set gradient uniforms
        int gradient_point_count = std::min((int)internal::background_gradient.positions.size(), 8);
        glUniform1i(glGetUniformLocation(internal::background_program, "u_gradient_point_count"), gradient_point_count);

        float positions[8] = {0};
        glm::vec4 colors[8] = {glm::vec4(0, 0, 0, 1)};
        for (int i = 0; i < gradient_point_count; ++i)
        {
            positions[i] = internal::background_gradient.positions[i];
            colors[i] = internal::background_gradient.colors[i];
        }
        glUniform1fv(glGetUniformLocation(internal::background_program, "u_gradient_positions"), 8, positions);
        glUniform4fv(glGetUniformLocation(internal::background_program, "u_gradient_colors"), 8, glm::value_ptr(colors[0]));

        // Render full-screen quad
        glBindVertexArray(internal::background_vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        drawCalls++;

        // Re-enable depth test
        glEnable(GL_DEPTH_TEST);
    }
}

void xsr::enable_depth_test(const bool state)
{
    if (state)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

void xsr::render_grid(const float* view, const float* projection, const shader_handle&)
{
    if (internal::m_grid_settings.showGrid)
    {
        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
        // Disable face culling
        glDisable(GL_CULL_FACE);

        // **Render the Grid**
        glUseProgram(grid_program);

        // Set shader uniforms
        GLint model_loc = glGetUniformLocation(grid_program, "u_model");
        GLint view_loc = glGetUniformLocation(grid_program, "u_view");
        GLint proj_loc = glGetUniformLocation(grid_program, "u_projection");

        glm::mat4 model = glm::mat4(1.0f);  // Identity matrix or adjust as needed
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, view);
        glUniformMatrix4fv(proj_loc, 1, GL_FALSE, projection);

        // Set grid uniform parameters as needed using the grid settings
        glUniform3fv(glGetUniformLocation(grid_program, "u_major_line_color"), 1, &internal::m_grid_settings.majorLineColor[0]);
        glUniform3fv(glGetUniformLocation(grid_program, "u_minor_line_color"), 1, &internal::m_grid_settings.minorLineColor[0]);
        glUniform3fv(glGetUniformLocation(grid_program, "u_x_axis_line_color"),
                     1,
                     &internal::m_grid_settings.xAxisLineColor[0]);
        glUniform3fv(glGetUniformLocation(grid_program, "u_z_axis_line_color"),
                     1,
                     &internal::m_grid_settings.zAxisLineColor[0]);
        glUniform1f(glGetUniformLocation(grid_program, "u_grid_size"), internal::m_grid_settings.gridSize);
        glUniform1f(glGetUniformLocation(grid_program, "u_grid_spacing"), internal::m_grid_settings.gridSpacing);
        glUniform1f(glGetUniformLocation(grid_program, "u_major_line_threshold"), internal::m_grid_settings.majorLineThreshold);
        glUniform1f(glGetUniformLocation(grid_program, "u_minor_line_threshold"), internal::m_grid_settings.minorLineThreshold);

        // Bind the grid VAO and draw
        glBindVertexArray(grid_vao);
        glDrawElements(GL_TRIANGLES, grid_index_count, GL_UNSIGNED_INT, 0);
        drawCalls++;
        glBindVertexArray(0);
    }
}

void xsr::clear_screen()
{
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}

void xsr::internal::DrawInstancedGroup(const std::vector<InstanceData>& group,
                                       const mesh_handle mesh_handle,
                                       const texture_handle texture_handle,
                                       const GLint texture_location)
{
    Mesh& mesh = internal::meshes[mesh_handle.id - 1];
    Texture& texture = internal::textures[texture_handle.id - 1];

    // instanceData.reserve(group.second.size());
    // for (const Entry* entry : group.second)
    //{
    //     instanceData.push_back(
    //         {entry->transform, entry->mul_color, entry->add_color, static_cast<float>(entry->receive_shadows)});
    // }

    // Set up the instance VBO
    if (mesh.instanceVBO == 0)
    {
        glGenBuffers(1, &mesh.instanceVBO);
    }

    glBindBuffer(GL_ARRAY_BUFFER, mesh.instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, group.size() * sizeof(InstanceData), group.data(), GL_DYNAMIC_DRAW);

    //// Set up the instanced vertex attributes
    glBindVertexArray(mesh.vao);

    // Adjusted attribute locations
    GLsizei vec4Size = sizeof(glm::vec4);
    GLsizei instanceDataSize = sizeof(InstanceData);

    // a_model matrix attribute (locations 4-7)
    for (int i = 0; i < 4; i++)
    {
        glEnableVertexAttribArray(4 + i);
        glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, instanceDataSize, (void*)(uintptr_t)(i * vec4Size));
        glVertexAttribDivisor(4 + i, 1);
    }

    // a_mul_color (location 8)
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, instanceDataSize, (void*)(uintptr_t)(4 * vec4Size));
    glVertexAttribDivisor(8, 1);

    // a_add_color (location 9)
    glEnableVertexAttribArray(9);
    glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, instanceDataSize, (void*)(uintptr_t)(5 * vec4Size));
    glVertexAttribDivisor(9, 1);

    // a_receive_shadows (location 10)
    glEnableVertexAttribArray(10);
    glVertexAttribPointer(10, 1, GL_FLOAT, GL_FALSE, instanceDataSize, (void*)(uintptr_t)(6 * vec4Size));
    glVertexAttribDivisor(10, 1);

    // Unbind VAO and VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glUniform1i(texture_location, 0);

    // Draw the instances
    glBindVertexArray(mesh.vao);
    glDrawElementsInstanced(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, 0, (int)group.size());
    drawCalls++;
    glBindVertexArray(0);
}

unsigned int xsr::internal::use_shader(const xsr::shader_handle& shader)
{
    unsigned int program = 0;
    if (shader.is_valid())
    {
        program = internal::get_program(shader);
    }
    else
    {
        program = internal::standard_program;
    }
    glUseProgram(program);
    return program;
}

void xsr::clear_entries()
{
    internal::entries.clear();
    internal::dir_lights.clear();
    internal::point_lights.clear();
    ambient_light = {{0.0f, 0.0f, 0.0f, 0.0f}};
    internal::background_gradient_set = false;

    for (auto& instance : internal::instances)
    {
        instance.second.clear();
    }
    for (auto& instance : internal::transparentInstances)
    {
        instance.second.clear();
    }

    instances.clear();
    transparentInstances.clear();

#ifdef EDITOR_MODE
    m_linesCount = 0;
#endif
}

bool xsr::on_imgui_render()
{
    ImGui::Begin(ICON_FA_IMAGE TAB_FA "Render Stats");
    ImGui::Text("Draw Calls: %d", drawCalls);
    ImGui::Text("In reality it's double the drawcalls, one for the editor buffer other for the gameplay buffer");
    ImGui::End();
    return true;
}

// Create a  GLFW window
bool xsr::device::initialize(const device_configuration& config)
{
    dconfig = config;
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
#if defined(_DEBUG)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_FALSE);
#endif

    window = glfwCreateWindow(config.width, config.height, config.title.c_str(), nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        return false;
    }
    return true;
}

bool xsr::device::initialize(const device_configuration& config, void* _window)
{
    dconfig = config;

    window = static_cast<GLFWwindow*>(_window);

    return true;
}

bool xsr::device::initialize(bee::Device*) { return false; }

void xsr::device::shutdown() { glfwTerminate(); }

void xsr::device::update()
{
    glfwPollEvents();
    glfwSwapBuffers(window);
}

bool xsr::device::should_close() { return glfwWindowShouldClose(window); }

void* xsr::device::get_window() { return window; }

device_configuration& xsr::device::get_device_configuration() { return dconfig; }

render_configuration& xsr::device::get_render_configuration() { return rconfig; }

Shader xsr::internal::get_program(const shader_handle& handle)
{
    if (handle.id <= 0 || handle.id > (int)internal::shaders.size())
    {
        bee::Log::Critical("Invalid shader handle");
        return Shader();
    }
    return internal::shaders[handle.id - 1];
}

shader_handle xsr::create_shader(const char* vertexShader, const char* fragmentShader)
{
    Shader newShader;
    newShader.program = glCreateProgram();

    GLuint vert_shader = 0;
    GLuint frag_shader = 0;

    GLboolean res = compile_shader(&vert_shader, GL_VERTEX_SHADER, vertexShader);
    if (!res) return shader_handle();

    res = compile_shader(&frag_shader, GL_FRAGMENT_SHADER, fragmentShader);
    if (!res)
    {
        printf("Failed to compile fragment shader");
        return shader_handle();
    }

    glAttachShader(newShader.program, vert_shader);
    glAttachShader(newShader.program, frag_shader);

    if (!link_program(newShader.program))
    {
        glDeleteShader(vert_shader);
        glDeleteShader(frag_shader);
        glDeleteProgram(newShader.program);
        printf("Failed to link shader program");
        return shader_handle();
    }

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);

    // log any errors
    GLint success;
    glGetProgramiv(newShader.program, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[512];
        glGetProgramInfoLog(newShader.program, 512, NULL, infoLog);
        bee::Log::Error("Shader program linking failed: {}", infoLog);
    }

    internal::shaders.push_back(newShader);
    return shader_handle{(int)internal::shaders.size()};
}

void xsr::set_standard_shader(shader_handle shader) { internal::standard_program = internal::get_program(shader).program; }

// This shader has been written by ChatGPT
bool xsr::internal::compile_background_shader()
{
    const char* vs_str = R"(
    #version 460 core
    layout(location = 0) in vec2 a_position;
    out vec2 v_uv;
    void main() {
        v_uv = a_position * 0.5 + 0.5;
        gl_Position = vec4(a_position, 0.0, 1.0);
    }
    )";

    const char* fs_str = R"(
#version 460 core
in vec2 v_uv;
out vec4 frag_color;
uniform mat4 u_inv_view_proj; // Inverse of the View-Projection matrix
uniform vec3 u_camera_pos; // Camera position in world space
const int MAX_GRADIENT_POINTS = 8;
uniform int u_gradient_point_count;
uniform float u_gradient_positions[MAX_GRADIENT_POINTS];
uniform vec4 u_gradient_colors[MAX_GRADIENT_POINTS];

void main() {
    // Convert the screen-space coordinates to world-space direction
    vec4 clip_pos = vec4(v_uv * 2.0 - 1.0, 1.0, 1.0); // Convert UV to clip space
    vec4 world_pos = u_inv_view_proj * clip_pos; // Convert to world space
    vec3 world_dir = normalize(world_pos.xyz / world_pos.w - u_camera_pos); // Normalize the direction vector

    // Use the Y component of the view direction to calculate gradient position
    float t = clamp(world_dir.y * 0.5 + 0.5, 0.0, 1.0); // Normalize Y to range [0, 1]

    // Determine the gradient color based on t
    if (u_gradient_point_count == 0) {
        frag_color = vec4(0.0, 0.0, 0.0, 1.0); // Default color if no gradient
    } else if (t <= u_gradient_positions[0]) {
        frag_color = u_gradient_colors[0]; // Use the first color if t is less than the first position
    } else if (t >= u_gradient_positions[u_gradient_point_count - 1]) {
        frag_color = u_gradient_colors[u_gradient_point_count - 1]; // Use the last color if t exceeds the last position
    } else {
        // Linear interpolation between the gradient colors
        for (int i = 0; i < u_gradient_point_count - 1; ++i) {
            if (t >= u_gradient_positions[i] && t <= u_gradient_positions[i + 1]) {
                float tt = (t - u_gradient_positions[i]) / (u_gradient_positions[i + 1] - u_gradient_positions[i]);
                frag_color = mix(u_gradient_colors[i], u_gradient_colors[i + 1], tt);
                break;
            }
        }
    }
}
)";

    shader_handle shader = create_shader(vs_str, fs_str);
    if (shader.is_valid())
    {
        internal::background_program = internal::get_program(shader).program;
        return true;
    }

    return false;
}

bool xsr::internal::compile_grid_shader()
{
    const char* vs_str = R"(
    #version 460 core

layout(location = 0) in vec3 a_position;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec3 v_world_pos;

void main()
{
    vec4 world_pos = u_model * vec4(a_position, 1.0);
    v_world_pos = world_pos.xyz;
    gl_Position = u_projection * u_view * world_pos;
}

    )";

    const char* fs_str = R"(
#version 460 core

in vec3 v_world_pos;
out vec4 frag_color;

uniform vec3 u_major_line_color;
uniform vec3 u_minor_line_color;
uniform vec3 u_x_axis_line_color;
uniform vec3 u_z_axis_line_color;
uniform float u_grid_size;
uniform float u_grid_spacing;
uniform float u_major_line_threshold;
uniform float u_minor_line_threshold;

void main()
{


// Calculate distances to the nearest grid lines
    float major_line_x = mod(abs(v_world_pos.x), u_grid_size) < u_major_line_threshold ? 1.0 : 0.0;
    float major_line_z = mod(abs(v_world_pos.z), u_grid_size) < u_major_line_threshold ? 1.0 : 0.0;
    float minor_line_x = mod(abs(v_world_pos.x), u_grid_spacing) < u_minor_line_threshold ? 1.0 : 0.0;
    float minor_line_z = mod(abs(v_world_pos.z), u_grid_spacing) < u_minor_line_threshold ? 1.0 : 0.0;

    // Axis lines
    float x_axis_line = abs(v_world_pos.x) < u_major_line_threshold ? 1.0 : 0.0;
    float z_axis_line = abs(v_world_pos.z) < u_major_line_threshold ? 1.0 : 0.0;

    // Determine color
    vec3 color = vec3(0.0);
    bool is_line = false;

    // Check for axis lines first
    if (x_axis_line == 1.0 && z_axis_line == 1.0) {
        // Intersection of X and Z axes
        color = (u_x_axis_line_color + u_z_axis_line_color) * 0.5;
        is_line = true;
    } else if (x_axis_line == 1.0) {
        color = u_x_axis_line_color;
        is_line = true;
    } else if (z_axis_line == 1.0) {
        color = u_z_axis_line_color;
        is_line = true;
    }
    // Then check for major lines
    else if (major_line_x == 1.0 || major_line_z == 1.0) {
        color = u_major_line_color;
        is_line = true;
    }
    // Then check for minor lines
    else if (minor_line_x == 1.0 || minor_line_z == 1.0) {
        color = u_minor_line_color;
        is_line = true;
    }

    // Discard fragment if not on any line
    if (!is_line)
        discard;

    frag_color = vec4(color, 1.0);
}
    )";

    shader_handle shader = create_shader(vs_str, fs_str);
    if (shader.is_valid())
    {
        internal::grid_program = internal::get_program(shader).program;
        return true;
    }

    return false;
}

void xsr::internal::create_grid_mesh()
{
    float grid_extent = 100.0f;  // Adjust as needed
    float vertices[] = {
        -grid_extent,
        0.0f,
        -grid_extent,
        grid_extent,
        0.0f,
        -grid_extent,
        grid_extent,
        0.0f,
        grid_extent,
        -grid_extent,
        0.0f,
        grid_extent,
    };

    unsigned int indices[] = {0, 1, 2, 2, 3, 0};
    grid_index_count = 6;

    glGenVertexArrays(1, &grid_vao);
    glBindVertexArray(grid_vao);

    glGenBuffers(1, &grid_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, grid_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &grid_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);  // Make sure this matches the layout in your shader
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}

void xsr::internal::create_fullscreen_quad()
{
    float vertices[] = {
        -1.0f,
        -1.0f,  // bottom-left
        1.0f,
        -1.0f,  // bottom-right
        -1.0f,
        1.0f,  // top-left
        1.0f,
        1.0f  // top-right
    };
    unsigned int indices[] = {0, 1, 2, 1, 3, 2};
    glGenVertexArrays(1, &background_vao);
    glBindVertexArray(background_vao);
    unsigned int vbo, ebo;
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

bool xsr::internal::compile_shader(GLuint* shader, GLenum type, const GLchar* source)
{
    GLint status;

    if (!source)
    {
        printf("Failed to compile empty shader");
        return false;
    }

    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, nullptr);

    glCompileShader(*shader);

#if defined(_DEBUG)
    GLint log_length = 0;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length > 1)
    {
        GLchar* log = static_cast<GLchar*>(malloc(log_length));
        glGetShaderInfoLog(*shader, log_length, &log_length, log);
        if (log) printf("Program compile log: %s", log);
        free(log);
    }
#endif

    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == 0)
    {
        glDeleteShader(*shader);
        return false;
    }

    return true;
}

bool xsr::internal::link_program(GLuint program)
{
    GLint status;
    glLinkProgram(program);

#if defined(_DEBUG)
    GLint logLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        GLchar* log = static_cast<GLchar*>(malloc(logLength));
        glGetProgramInfoLog(program, logLength, &logLength, log);
        if (log) printf("Program link log: %s", log);
        free(log);
    }
#endif

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    return status != 0;
}

void xsr::internal::glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

#ifdef EDITOR_MODE

#if defined(_WIN32)
void xsr::internal::init_debug_messages()
{
    // Query the OpenGL function to register your callback function.
    PFNGLDEBUGMESSAGECALLBACKPROC _glDebugMessageCallback =
        (PFNGLDEBUGMESSAGECALLBACKPROC)wglGetProcAddress("glDebugMessageCallback");
    PFNGLDEBUGMESSAGECALLBACKARBPROC _glDebugMessageCallbackARB =
        (PFNGLDEBUGMESSAGECALLBACKARBPROC)wglGetProcAddress("glDebugMessageCallbackARB");
    PFNGLDEBUGMESSAGECALLBACKAMDPROC _glDebugMessageCallbackAMD =
        (PFNGLDEBUGMESSAGECALLBACKAMDPROC)wglGetProcAddress("glDebugMessageCallbackAMD");

    glDebugMessageCallback(xsr::internal::debug_callback_func, nullptr);

    // Register your callback function.
    if (_glDebugMessageCallback != nullptr)
    {
        _glDebugMessageCallback(xsr::internal::debug_callback_func, nullptr);
    }
    else if (_glDebugMessageCallbackARB != nullptr)
    {
        _glDebugMessageCallbackARB(xsr::internal::debug_callback_func, nullptr);
    }

    // Additional AMD support
    if (_glDebugMessageCallbackAMD != nullptr)
    {
        _glDebugMessageCallbackAMD(debug_callback_func_amd, nullptr);
    }

    // Enable synchronous callback. This ensures that your callback function is called
    // right after an error has occurred. This capability is not defined in the AMD
    // version.
    if ((_glDebugMessageCallback != nullptr) || (_glDebugMessageCallbackARB != nullptr))
    {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }

    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_FALSE);

    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
}

void xsr::internal::init_debug_variables()
{
    m_vertexArray = new VertexPosition3DColor[m_maxLines * 2];

    const auto* const vsSource =
        "#version 460 core												\n\
		layout (location = 1) in vec3 a_position;						\n\
		layout (location = 2) in vec4 a_color;							\n\
		layout (location = 1) uniform mat4 u_worldviewproj;				\n\
		out vec4 v_color;												\n\
																		\n\
		void main()														\n\
		{																\n\
			v_color = a_color;											\n\
			gl_Position = u_worldviewproj * vec4(a_position, 1.0);		\n\
		}";

    const auto* const fsSource =
        "#version 460 core												\n\
		in vec4 v_color;												\n\
		out vec4 frag_color;											\n\
																		\n\
		void main()														\n\
		{																\n\
			frag_color = v_color;										\n\
		}";

    GLuint vertShader = 0;
    GLuint fragShader = 0;
    GLboolean res = GL_FALSE;

    debug_program = glCreateProgram();

    res = compile_shader(&vertShader, GL_VERTEX_SHADER, vsSource);
    if (!res)
    {
        printf("DebugRenderer failed to compile vertex shader");
        return;
    }

    res = compile_shader(&fragShader, GL_FRAGMENT_SHADER, fsSource);
    if (!res)
    {
        printf("DebugRenderer failed to compile fragment shader");
        return;
    }

    glAttachShader(debug_program, vertShader);
    glAttachShader(debug_program, fragShader);

    if (!link_program(debug_program))
    {
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        glDeleteProgram(debug_program);
        printf("DebugRenderer failed to link shader program");
        return;
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    glCreateVertexArrays(1, &m_linesVAO);
    glBindVertexArray(m_linesVAO);

    // Allocate VBO
    glGenBuffers(1, &m_linesVBO);

    // Array buffer contains the attribute data
    glBindBuffer(GL_ARRAY_BUFFER, m_linesVBO);

    // Allocate into VBO
    const auto size = sizeof(m_vertexArray);
    glBufferData(GL_ARRAY_BUFFER, size, &m_vertexArray[0], GL_STREAM_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(VertexPosition3DColor),
                          reinterpret_cast<void*>(offsetof(VertexPosition3DColor, Position)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,
                          4,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(VertexPosition3DColor),
                          reinterpret_cast<void*>(offsetof(VertexPosition3DColor, Color)));

    glBindVertexArray(0);  // TODO: Only do this when validating OpenGL

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

#else
void xsr::internal::init_debug_messages() { glDebugMessageCallback(debug_callback_func, nullptr); }
#endif

void xsr::internal::debug_callback_func_amd(GLuint id,
                                            GLenum category,
                                            GLenum severity,
                                            GLsizei length,
                                            const GLchar* message,
                                            void* userParam)
{
    xsr::internal::debug_callback_func(GL_DEBUG_CATEGORY_API_ERROR_AMD, category, id, severity, length, message, userParam);
}

void xsr::internal::debug_callback_func(GLenum source,
                                        GLenum type,
                                        GLuint id,
                                        GLenum severity,
                                        GLsizei,
                                        const GLchar* message,
                                        const GLvoid*)
{
    // Skip some less useful info
    if (id == 131218)  // http://stackoverflow.com/questions/12004396/opengl-debug-context-performance-warning
        return;

    // UNUSED(length);
    // UNUSED(userParam);
    std::string source_string;
    std::string type_string;
    std::string severity_string;

    // The AMD variant of this extension provides a less detailed classification of the error,
    // which is why some arguments might be "Unknown".
    switch (source)
    {
        case GL_DEBUG_CATEGORY_API_ERROR_AMD:
        case GL_DEBUG_SOURCE_API:
        {
            source_string = "API";
            break;
        }
        case GL_DEBUG_CATEGORY_APPLICATION_AMD:
        case GL_DEBUG_SOURCE_APPLICATION:
        {
            source_string = "Application";
            break;
        }
        case GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD:
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        {
            source_string = "Window System";
            break;
        }
        case GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD:
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
        {
            source_string = "Shader Compiler";
            break;
        }
        case GL_DEBUG_SOURCE_THIRD_PARTY:
        {
            source_string = "Third Party";
            break;
        }
        case GL_DEBUG_CATEGORY_OTHER_AMD:
        case GL_DEBUG_SOURCE_OTHER:
        {
            source_string = "Other";
            break;
        }
        default:
        {
            source_string = "Unknown";
            break;
        }
    }

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:
        {
            type_string = "Error";
            break;
        }
        case GL_DEBUG_CATEGORY_DEPRECATION_AMD:
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        {
            type_string = "Deprecated Behavior";
            break;
        }
        case GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD:
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        {
            type_string = "Undefined Behavior";
            break;
        }
        case GL_DEBUG_TYPE_PORTABILITY_ARB:
        {
            type_string = "Portability";
            break;
        }
        case GL_DEBUG_CATEGORY_PERFORMANCE_AMD:
        case GL_DEBUG_TYPE_PERFORMANCE:
        {
            type_string = "Performance";
            break;
        }
        case GL_DEBUG_CATEGORY_OTHER_AMD:
        case GL_DEBUG_TYPE_OTHER:
        {
            type_string = "Other";
            break;
        }
        default:
        {
            type_string = "Unknown";
            break;
        }
    }

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
        {
            severity_string = "High";
            break;
        }
        case GL_DEBUG_SEVERITY_MEDIUM:
        {
            severity_string = "Medium";
            break;
        }
        case GL_DEBUG_SEVERITY_LOW:
        {
            severity_string = "Low";
            break;
        }
        default:
        {
            severity_string = "Unknown";
            return;
        }
    }

    printf("%s:%s(%s): %s\n", source_string.c_str(), type_string.c_str(), severity_string.c_str(), message);
}
#else
void xsr::internal::init_debug_messages() {}
void xsr::internal::init_debug_variables() {}
#endif
