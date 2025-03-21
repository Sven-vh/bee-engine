#pragma once

#include "common.hpp"
#include "tools/gradient.hpp"
#include "core/device.hpp"

namespace xsr
{
typedef unsigned char uchar;

/// <summary>
/// Handle for meshes and textures, used for type safety
/// </summary>
struct handle
{
    int id = 0;
    bool operator==(const handle& other) const { return id == other.id; }
    bool operator!=(const handle& other) const { return id != other.id; }
    bool is_valid() const { return id != 0; }
};

/// <summary>
/// Handle for meshes.
/// </summary>
struct mesh_handle : public handle
{
    mesh_handle() = default;
    mesh_handle(int id) { this->id = id; }
    glm::vec3 meshSize = glm::vec3(1.0f);
    glm::vec3 meshCenter = glm::vec3(0.0f);
};

/// <summary>
/// Handle for textures.
/// </summary>
struct texture_handle : public handle
{
    int width = 0;
    int height = 0;
};

struct shader_handle : public handle
{
};

/// <summary>
/// Used to configure the device (window) on all platforms.
/// </summary>
struct device_configuration
{
    int width = 0;
    int height = 0;
    std::string title;
};

/// <summary>
/// Used to configure the renderer
/// </summary>
struct render_configuration
{
    enum class texture_filtering
    {
        nearest,
        linear
    };

    bool enable_shadows = true;
    int shadow_resolution = 1024;
    texture_filtering texture_filter = texture_filtering::linear;
    std::string shader_path = "../../";
};

struct grid_settings
{
    // Member variables for the grid settings using glm::vec3 for colors
    bool showGrid = true;
    glm::vec3 majorLineColor = glm::vec3(0.3f, 0.3f, 0.3f);
    glm::vec3 minorLineColor = glm::vec3(0.5f, 0.5f, 0.5f);
    glm::vec3 xAxisLineColor = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 zAxisLineColor = glm::vec3(0.0f, 0.0f, 1.0f);
    float gridSize = 10.0f;
    float gridSpacing = 1.0f;
    float majorLineThreshold = 0.05f;
    float minorLineThreshold = 0.02f;

    // Function to render the ImGui window for changing settings
    void OnImGuiRender()
    {
        ImGui::Text("Grid Settings");
        ImGui::Checkbox("Show Grid", &showGrid);
        ImGui::ColorEdit3("Major Line Color", &majorLineColor[0]);
        ImGui::ColorEdit3("Minor Line Color", &minorLineColor[0]);
        ImGui::ColorEdit3("X Axis Line Color", &xAxisLineColor[0]);
        ImGui::ColorEdit3("Z Axis Line Color", &zAxisLineColor[0]);
        ImGui::SliderFloat("Grid Size", &gridSize, 1.0f, 100.0f);
        ImGui::SliderFloat("Grid Spacing", &gridSpacing, 0.1f, 10.0f);
        ImGui::SliderFloat("Major Line Threshold", &majorLineThreshold, 0.01f, 0.1f);
        ImGui::SliderFloat("Minor Line Threshold", &minorLineThreshold, 0.01f, 0.1f);
    }
};

grid_settings& get_grid_settings();

/// <summary>
/// Initialized the renderer (buffers for rendering, shaders etc)
/// </summary>
bool initialize(const render_configuration& config);

/// <summary>
/// Shutdown the renderer (clear the buffers for rendering, shaders etc)
/// </summary>
void shutdown();

/// <summary>
/// Creates a mesh from vertex data.
/// Must provide all attributes and the same number of vertices for each attribute.
/// </summary>
/// <param name="indices">Index buffer</param>
/// <param name="index_count">Index count</param>
/// <param name="positions">Position buffer. Three elements per position.</param>
/// <param name="normals">Normal buffer. Should be normalized already. Three elements per normal.</param>
/// <param name="texture_coordinates">Texture coordinates. Three elements per normal.</param>
/// <param name="colors">Vertex colors. Three elements per color in [0,1] range.</param>
/// <param name="vertex_count">The number of vertices. This must the same for all attributes.</param>
/// <returns>A mesh handle</returns>
mesh_handle create_mesh(const unsigned int* indices,
                        unsigned int index_count,
                        const float* positions,
                        const float* normals,
                        const float* texture_coordinates,
                        const float* colors,
                        unsigned int vertex_count);

/// <summary>
/// Unloads a mesh from memory.
/// </summary>
void unload_mesh(mesh_handle mesh);

/// <summary>
/// Creates a texture from pixel data.
/// </summary>
texture_handle create_texture(int width, int height, const void* pixel_rgba_bytes);

/// <summary>
/// Unloads a texture from memory.
/// </summary>
void unload_texture(texture_handle texture);

void* get_texture_id(texture_handle texture);

shader_handle create_shader(const char* vertexShader, const char* fragmentShader);

void set_standard_shader(shader_handle shader);

void set_background_gradient(const bee::ColorGradient& gradient);

void enable_wireframes(bool enable);

/// <summary>
/// Renders a directional light.
/// </summary>
/// <param name="direction">The direction of the light. Should be normalized.</param>
/// <param name="color_and_intensity">Four components, the last one is the intensity.</param>
bool render_directional_light(const float* direction, const float* color_and_intensity);

/// <summary>
/// Renders a point light.
/// </summary>
/// <param name="position">The position of the light.</param>
/// <param name="radius">The radius of influence of the light.</param>
/// <param name="color_and_intensity">Four components, the last one is the intensity.</param>
bool render_point_light(const float* position, float radius, const float* color_and_intensity);

/// <summary>
/// Renders an ambient light.
/// </summary>
/// <param name="color_and_intensity">Four components, the last one is the intensity.</param>
bool render_ambient_light(const float* color_and_intensity);

/// <summary>
/// Render a mesh with a texture.
/// </summary>
/// <param name="transform">The transform matrix.</param>
/// <param name="mesh">The mesh to render.</param>
/// <param name="texture">The texture to use.</param>
/// <param name="mul_color">Multiply color. Four components.</param>
/// <param name="add_color">Add color. Four components.</param>
bool render_mesh(const float* transform,
                 mesh_handle mesh,
                 texture_handle texture,
                 const float* mul_color = nullptr,
                 const float* add_color = nullptr,
                 const bool receive_shadows = true);

/// <summary>
/// Render a debug line.
/// </summary>
/// <param name="from">The start of the line.</param>
/// <param name="to">The end of the line.</param>
/// <param name="color">The color of the line. Four components.</param>
/// <returns>True if the line can be rendered.</returns>
bool render_debug_line(const float* from, const float* to, const float* color);

/// <summary>
/// Render debug text.
/// </summary>
/// <param name="text">The text to render.</param>
/// <param name="position">The position of the text.</param>
/// <param name="color">The color of the text. Four components.</param>
/// <param name="size">The size of the text.</param>
/// <returns>True if the text can be rendered.</returns>
bool render_debug_text(const std::string& text, const float* position, const float* color, float size);

/// <summary>
/// Render a debug cone.
/// </summary>
/// <param name="position">The position of the cone.</param>
/// <param name="direction">The direction of the cone.</param>
/// <param name="angle">The angle of the cone.</param>
/// <param name="color">The color of the cone. Four components.</param>
/// <returns>True if the cone can be rendered.</returns>
bool render_debug_cone(const float* position, const float* direction, float angle, const float* color);

/// <summary>
/// Render the scene.
/// </summary>
/// <param name="view">The view matrix.</param>
/// <param name="projection">The projection matrix.</param>
void render(const float* view, const float* projection, const shader_handle& shader = shader_handle());

void render_background(const float* view, const float* projection, const shader_handle& shader = shader_handle());

void enable_depth_test(const bool state);

void clear_screen();

void render_grid(const float* view, const float* projection, const shader_handle& shader = shader_handle());


/// <summary>
/// Clear the entries list.
/// </summary>
void clear_entries();

/// <summary>
/// Device specific functions. Used when the engine does not provide the functionality.
/// </summary>
namespace device
{
/// <summary>
/// Initialize the device
/// Will create a window with the given width, height and title.
/// Creates backbuffers on all platforms. Ready for rendering.
/// </summary>
bool initialize(const device_configuration& config);

/// <summary>
/// Initialize the device
/// Will the provided window.
/// Creates backbuffers on all platforms. Ready for rendering.
/// </summary>
bool initialize(const device_configuration& config, void* window);

/// <summary>
/// intialize xsr with the provided device.
/// </summary>
bool initialize(bee::Device* device);

/// <summary>
/// Shutdown the device. Destroys the window and backbuffers.
/// </summary>
void shutdown();

/// <summary>
/// Updates the device. Polls the window for events and swaps the backbuffer.
/// </summary>
void update();

/// <summary>
/// Returns true if the window is open.
/// </summary>
bool should_close();

/// <summary>
/// Returns a platform-dependent window pointer. It's up to the user to cast it to the right type.
/// </summary>
void* get_window();

device_configuration& get_device_configuration();

render_configuration& get_render_configuration();

}  // namespace device

bool on_imgui_render();

/// <summary>
/// A handful of tools to load meshes and textures.
/// </summary>
namespace tools
{
/// <summary>
/// Loads an obj mesh from a string. Only supports triangles.
/// </summary>
mesh_handle load_obj_mesh(const std::string& obj_file_contents, const std::string& object_name = "");

/// <summary>
/// Loads a png texture from a vector of bytes.
/// </summary>
texture_handle load_png_texture(const std::vector<char>& png_file_contents);
}  // namespace tools

}  // namespace xsr
