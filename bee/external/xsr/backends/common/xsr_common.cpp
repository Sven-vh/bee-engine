#include <sstream>
#include <map>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
// #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "xsr/include/xsr.hpp"

using namespace xsr;
using namespace std;
using namespace glm;

// this function had some bugs in it. Like not being able to support 200//200. where the middle number is missing.
// and not being able to support if some of the info is missing like normals or texture coordinates.
// it also didn't allow for .objs contain quads instead of triangles
// So using ChaptGPT I was able to fix this code to basically load every obj file.
mesh_handle tools::load_obj_mesh(const std::string& data, const std::string& object_name)
{
    if (data.empty()) return mesh_handle();

    std::stringstream ss(data);
    std::string line;

    std::vector<vec3> positions;
    std::vector<vec2> texcoords;
    std::vector<vec3> normals;
    std::vector<vec3> colors;

    std::vector<vec3> final_positions;
    std::vector<vec2> final_texcoords;
    std::vector<vec3> final_normals;
    std::vector<vec3> final_colors;
    std::vector<unsigned> final_indices;

    unsigned idx = 0;
    bool search = !object_name.empty();
    bool found = !search;

    int first_position = 0;
    int first_texcoord = 0;
    int first_normal = 0;

    vec3 min_bound(FLT_MAX, FLT_MAX, FLT_MAX);
    vec3 max_bound(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    while (std::getline(ss, line))
    {
        std::stringstream line_stream(line);
        std::string type;
        line_stream >> type;

        if (search && type == "o")
        {
            if (!found)
            {
                std::string name;
                line_stream >> name;
                found = (name == object_name);
            }
            else
            {
                break;
            }
        }

        if (type == "#" || type.empty())
        {
            continue;
        }
        else if (type == "v")
        {
            float x, y, z;
            line_stream >> x >> y >> z;
            if (found)
            {
                vec3 position(x, y, z);
                positions.emplace_back(position);

                min_bound = glm::min(min_bound, position);
                max_bound = glm::max(max_bound, position);
            }
            else
            {
                first_position++;
            }
        }
        else if (type == "vt")
        {
            float u, v;
            line_stream >> u >> v;
            if (found)
                texcoords.emplace_back(u, v);
            else
                first_texcoord++;
        }
        else if (type == "vn")
        {
            float x, y, z;
            line_stream >> x >> y >> z;
            if (found)
                normals.emplace_back(x, y, z);
            else
                first_normal++;
        }
        else if (type == "vc")
        {
            float r, g, b;
            line_stream >> r >> g >> b;
            colors.emplace_back(r, g, b);
        }
        else if (type == "f" && found)
        {
            std::vector<unsigned> face_indices;
            std::string vertex_data;

            while (line_stream >> vertex_data)
            {
                std::replace(vertex_data.begin(), vertex_data.end(), '/', ' ');
                std::stringstream vertex_stream(vertex_data);

                int v_idx = 0, vt_idx = 0, vn_idx = 0;
                vertex_stream >> v_idx;

                if (!vertex_stream.eof()) vertex_stream >> vt_idx;
                if (!vertex_stream.eof()) vertex_stream >> vn_idx;

                v_idx = (v_idx > 0 ? v_idx - 1 - first_position : 0);
                vt_idx = (vt_idx > 0 ? vt_idx - 1 - first_texcoord : -1);
                vn_idx = (vn_idx > 0 ? vn_idx - 1 - first_normal : -1);

                if (v_idx >= 0 && v_idx < positions.size()) final_positions.push_back(positions[v_idx]);
                if (vt_idx >= 0 && vt_idx < texcoords.size())
                    final_texcoords.push_back(texcoords[vt_idx]);
                else
                    final_texcoords.push_back(vec2(0.0f, 0.0f));
                if (vn_idx >= 0 && vn_idx < normals.size()) final_normals.push_back(normals[vn_idx]);

                final_colors.push_back(vec3(1, 1, 1));
                final_indices.push_back(idx++);
                face_indices.push_back(idx - 1);
            }

            if (face_indices.size() == 4)
            {
                final_indices.push_back(face_indices[0]);
                final_indices.push_back(face_indices[1]);
                final_indices.push_back(face_indices[2]);

                final_indices.push_back(face_indices[0]);
                final_indices.push_back(face_indices[2]);
                final_indices.push_back(face_indices[3]);
            }
            else
            {
                for (size_t i = 1; i + 1 < face_indices.size(); ++i)
                {
                    final_indices.push_back(face_indices[0]);
                    final_indices.push_back(face_indices[i]);
                    final_indices.push_back(face_indices[i + 1]);
                }
            }
        }
    }

    mesh_handle handle = create_mesh(final_indices.data(),
                                     static_cast<unsigned>(final_indices.size()),
                                     final_positions.empty() ? nullptr : value_ptr(final_positions[0]),
                                     final_normals.empty() ? nullptr : value_ptr(final_normals[0]),
                                     final_texcoords.empty() ? nullptr : value_ptr(final_texcoords[0]),
                                     final_colors.empty() ? nullptr : value_ptr(final_colors[0]),
                                     static_cast<unsigned>(final_positions.size()));

    vec3 mesh_size = max_bound - min_bound;
    handle.meshSize = mesh_size;

    return handle;
}

texture_handle tools::load_png_texture(const std::vector<char>& data)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    auto pixels = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(data.data()),
                                        (int)data.size(),
                                        &width,
                                        &height,
                                        &channels,
                                        4);
    stbi_set_flip_vertically_on_load(false);
    if (!pixels) return texture_handle();
    auto handle = create_texture(width, height, pixels);
    handle.width = width;
    handle.height = height;
    stbi_image_free(pixels);
    return handle;
}
