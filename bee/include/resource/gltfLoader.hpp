#pragma once
#include "common.hpp"
#include "resource/resourceManager.hpp"
#include "tinygltf/tiny_gltf.h"
#include "xsr/include/xsr.hpp"
#include <entt/entt.hpp>
#include <string>
#include <tuple>
#include <vector>
#include <glm/glm.hpp>

// forward declaration
namespace bee
{
struct GltfScene;
}

namespace bee::resource
{
entt::entity LoadGLTF(const fs::path& path, entt::registry& registry);
entt::entity LoadGLTFFromModel(entt::registry& registry,
                               const tinygltf::Model& model,
                               const fs::path& newFilePath = fs::path());
void LoadGLTFFromEntity(const entt::entity& sceneEntity, entt::registry& registry);

void LogGLTFWarningsAndErrors(const std::string& warn, const std::string& err, const fs::path& filePath);

std::vector<unsigned int> ConvertIndicesToUnsignedInt(const tinygltf::Accessor& indexAccessor, const tinygltf::Model& model);
Ref<bee::resource::Mesh> CreateMeshHandle(const tinygltf::Model& model,
                                          const tinygltf::Primitive& primitive,
                                          const std::string& name);
Ref<bee::resource::Texture> CreateTextureHandle(const tinygltf::Model& model,
                                                const std::string& name,
                                                const tinygltf::Primitive& primitive);
std::tuple<glm::vec4, glm::vec4> GetMaterialColors(const tinygltf::Model& model, const tinygltf::Primitive& primitive);

// Checks that the data layout has the expected number of components
template <typename T>
const T* GetAttributeData(const tinygltf::Accessor& accessor, const tinygltf::Model& model, int expectedComponentCount)
{
    if (accessor.type != expectedComponentCount)
    {
        throw std::runtime_error("Unexpected number of components in attribute data.");
    }

    const auto& bufferView = model.bufferViews[accessor.bufferView];
    const auto& buffer = model.buffers[bufferView.buffer];
    return reinterpret_cast<const T*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
}
}  // namespace bee::resource