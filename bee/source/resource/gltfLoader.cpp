#include "resource/gltfLoader.hpp"
#include <tinygltf/tiny_gltf.h>
#include "core.hpp"

entt::entity bee::resource::LoadGLTF(const fs::path& filePath, entt::registry& registry)
{
    auto newFilePath = bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Root, filePath);
    tinygltf::Model model = bee::resource::LoadResource<GltfModel>(newFilePath)->GetModel();

    return LoadGLTFFromModel(registry, model, newFilePath);
}

entt::entity bee::resource::LoadGLTFFromModel(entt::registry& registry,
                                              const tinygltf::Model& model,
                                              const fs::path& newFilePath)
{
    // this will be the root entity for the GLTF scene
    entt::entity gltfSceneEntity = bee::ecs::CreateDefault(bee::FileIO::GetFileName(newFilePath));
    registry.emplace<GltfScene>(gltfSceneEntity);
    GltfScene& gltfScene = registry.get<GltfScene>(gltfSceneEntity);

    gltfScene.name = model.scenes[model.defaultScene].name;
    gltfScene.path = bee::Engine.FileIO().GetRelativePath(bee::FileIO::Directory::Root, newFilePath);

    std::unordered_map<int, entt::entity> nodeEntityMap;

    // Iterate through the nodes in the GLTF model to create entities and hierarchy
    for (size_t i = 0; i < model.nodes.size(); ++i)
    {
        const auto& node = model.nodes[i];
        entt::entity nodeEntity = bee::ecs::CreateDefault(node.name.empty() ? "Unnamed Node" : node.name);

        registry.emplace<GltfNode>(nodeEntity);
        GltfNode& gltfNode = registry.get<GltfNode>(nodeEntity);
        gltfNode.name = node.name;
        gltfNode.meshId = node.mesh;

        // Create and assign the Transform component
        Transform& transform = registry.get<Transform>(nodeEntity);
        if (!node.translation.empty())
        {
            transform.SetPosition(
                glm::vec3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]));
        }
        if (!node.rotation.empty())
        {
            glm::quat rotation =
                glm::quat((float)node.rotation[3], (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2]);
            transform.SetRotationQuat(rotation);
        }
        if (!node.scale.empty())
        {
            transform.SetScale(glm::vec3((float)node.scale[0], (float)node.scale[1], (float)node.scale[2]));
        }

        nodeEntityMap[(int)i] = nodeEntity;

        // If the node has a mesh, add a Renderable component
        if (node.mesh >= 0)
        {
            const auto& mesh = model.meshes[node.mesh];

            int primitiveIndex = 0;
            bool onePrimitive = mesh.primitives.size() == 1;
            for (const auto& primitive : mesh.primitives)
            {
                // Create mesh and texture handles
                Ref<bee::resource::Mesh> meshHandle =
                    CreateMeshHandle(model, primitive, mesh.name + std::to_string(primitiveIndex));
                Ref<bee::resource::Texture> textureHandle =
                    CreateTextureHandle(model, mesh.name + std::to_string(primitiveIndex), primitive);

                // Get multiplier and tint from the material
                auto [multiplier, tint] = GetMaterialColors(model, primitive);

                // Assign Renderable component
                if (onePrimitive)
                {
                    registry.emplace<Renderable>(nodeEntity,
                                                 Renderable{meshHandle, textureHandle, multiplier, tint, true, false});
                }
                else
                {
                    entt::entity primitiveEntity = bee::ecs::CreateEmpty();
                    registry.emplace<Transform>(primitiveEntity);
                    registry.emplace<HierarchyNode>(primitiveEntity, "Primitive " + std::to_string(primitiveIndex));
                    bee::ecs::SetParentChildRelationship(primitiveEntity, nodeEntity, registry, true);
                    registry.emplace<Renderable>(primitiveEntity,
                                                 Renderable{meshHandle, textureHandle, multiplier, tint, true, false});
                }
                // registry.emplace<Renderable>(nodeEntity,
                //                              Renderable{meshHandle, textureHandle, multiplier, tint, true, false});

                primitiveIndex++;
            }
        }
    }

    // After creating all entities, establish parent-child relationships
    for (size_t i = 0; i < model.nodes.size(); ++i)
    {
        const auto& node = model.nodes[i];
        entt::entity entity = nodeEntityMap[(int)i];

        auto& hierarchyNode = registry.get<HierarchyNode>(entity);
        hierarchyNode.selectParent = true;
        if (!node.children.empty())
        {
            for (int childIndex : node.children)
            {
                entt::entity childEntity = nodeEntityMap[childIndex];
                hierarchyNode.children.push_back(childEntity);

                auto& childHierarchyNode = registry.get<HierarchyNode>(childEntity);
                childHierarchyNode.parent = entity;
                childHierarchyNode.selectParent = true;
            }
        }
    }

    // loop through every node and if it doesnot have a parent, it is a root node, so add it to the scene
    for (size_t i = 0; i < model.nodes.size(); ++i)
    {
        entt::entity entity = nodeEntityMap[(int)i];
        HierarchyNode& hierarchyNode = registry.get<HierarchyNode>(entity);

        if (hierarchyNode.parent == entt::null)
        {
            hierarchyNode.parent = gltfSceneEntity;

            HierarchyNode& gltfSceneHierarchyNode = registry.get<HierarchyNode>(gltfSceneEntity);
            gltfSceneHierarchyNode.children.push_back(entity);
        }
    }

    return gltfSceneEntity;
}

void bee::resource::LoadGLTFFromEntity(const entt::entity& sceneEntity, entt::registry& registry)
{
    GltfScene& gltfScene = registry.get<GltfScene>(sceneEntity);
    if (gltfScene.path.empty())
    {
        return;
    }
    // Load the GLTF model using the path from GltfScene component
    tinygltf::Model model =
        bee::resource::LoadResource<GltfModel>(bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Root, gltfScene.path))
            .get()
            ->GetModel();

    // Update the GltfScene component with the loaded data
    gltfScene.name = model.scenes[model.defaultScene].name;

    // Create a map to associate node names with entities
    std::unordered_map<std::string, entt::entity> nodeEntityMap;

    // Collect all entities that have a GltfNode component and map them by name
    auto view = bee::ecs::GetAllChildrenWithComponent<GltfNode>(registry, sceneEntity);
    for (auto entity : view)
    {
        GltfNode& gltfNode = registry.get<GltfNode>(entity);
        if (!gltfNode.name.empty())
        {
            nodeEntityMap[gltfNode.name] = entity;
        }
    }

    // Now, iterate through the nodes in the GLTF model and update existing entities
    for (size_t i = 0; i < model.nodes.size(); ++i)
    {
        const auto& node = model.nodes[i];
        entt::entity nodeEntity = entt::null;

        // Find the corresponding entity using node name
        if (!node.name.empty() && nodeEntityMap.find(node.name) != nodeEntityMap.end())
        {
            nodeEntity = nodeEntityMap[node.name];
        }
        else
        {
            bee::Log::Warn("No matching entity found for GLTF node name: {}", node.name);
            continue;
        }

        // Update the Transform component
        if (registry.all_of<Transform>(nodeEntity))
        {
            Transform& transform = registry.get<Transform>(nodeEntity);
            if (!node.translation.empty())
            {
                transform.SetPosition(
                    glm::vec3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]));
            }
            if (!node.rotation.empty())
            {
                glm::quat rotation = glm::quat((float)node.rotation[3],
                                               (float)node.rotation[0],
                                               (float)node.rotation[1],
                                               (float)node.rotation[2]);
                transform.SetRotationQuat(rotation);
            }
            if (!node.scale.empty())
            {
                transform.SetScale(glm::vec3((float)node.scale[0], (float)node.scale[1], (float)node.scale[2]));
            }
        }

        // loop through all the children
        HierarchyNode& nodeHierarchy = registry.get<HierarchyNode>(nodeEntity);
        for (auto it = nodeHierarchy.children.begin(); it != nodeHierarchy.children.end();)
        {
            entt::entity child = *it;
            if (!registry.valid(child))
            {
                // remove it from the parent's children list
                it = nodeHierarchy.children.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // If the node has a mesh, update the Renderable component
        if (node.mesh >= 0)
        {
            const auto& mesh = model.meshes[node.mesh];
            bool onePrimitive = mesh.primitives.size() == 1;

            int primitiveIndex = 0;
            for (const auto& primitive : mesh.primitives)
            {
                // Create mesh and texture handles
                Ref<bee::resource::Mesh> meshHandle =
                    CreateMeshHandle(model, primitive, mesh.name + std::to_string(primitiveIndex));
                Ref<bee::resource::Texture> textureHandle =
                    CreateTextureHandle(model, mesh.name + std::to_string(primitiveIndex), primitive);

                // Get multiplier and tint from the material
                auto [multiplier, tint] = GetMaterialColors(model, primitive);

                if (onePrimitive)
                {
                    if (registry.all_of<Renderable>(nodeEntity))
                    {
                        Renderable& renderable = registry.get<Renderable>(nodeEntity);
                        renderable.mesh = meshHandle;
                        renderable.texture = textureHandle;
                        renderable.multiplier =
                            (renderable.multiplier == DEFAULT_MULTIPLY_COLOR) ? multiplier : renderable.multiplier;
                        renderable.tint = (renderable.tint == DEFAULT_TINT_COLOR) ? tint : renderable.tint;
                    }
                    else
                    {
                        registry.emplace<Renderable>(nodeEntity,
                                                     Renderable{meshHandle, textureHandle, multiplier, tint, true, false});
                    }
                }
                else
                {
                    entt::entity primitiveEntity = bee::ecs::CreateEmpty();
                    registry.emplace<Transform>(primitiveEntity);
                    registry.emplace<HierarchyNode>(primitiveEntity, "Primitive" + std::to_string(primitiveIndex));
                    bee::ecs::SetParentChildRelationship(primitiveEntity, nodeEntity, registry, true);
                    registry.emplace<Renderable>(primitiveEntity,
                                                 Renderable{meshHandle, textureHandle, multiplier, tint, true, false});
                }
                primitiveIndex++;
            }
        }
    }
}
// Logs warnings and errors from loading GLTF
void bee::resource::LogGLTFWarningsAndErrors(const std::string& warn, const std::string& err, const fs::path& filePath)
{
    if (!warn.empty())
    {
        bee::Log::Warn("Warning while loading GLTF: {}", warn);
    }
    if (!err.empty())
    {
        bee::Log::Error("Error while loading GLTF: {}", err);
    }
    if (!err.empty() || !warn.empty())
    {
        bee::Log::Error("Failed to load GLTF: {}", filePath.string());
    }
}

// Converts GLTF index data to unsigned int if needed, got it from chatGPT
std::vector<unsigned int> bee::resource::ConvertIndicesToUnsignedInt(const tinygltf::Accessor& indexAccessor,
                                                                     const tinygltf::Model& model)
{
    std::vector<unsigned int> indices(indexAccessor.count);

    const auto& bufferView = model.bufferViews[indexAccessor.bufferView];
    const auto& buffer = model.buffers[bufferView.buffer];
    const void* data = &buffer.data[bufferView.byteOffset + indexAccessor.byteOffset];

    switch (indexAccessor.componentType)
    {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        {
            const auto* byteData = reinterpret_cast<const unsigned char*>(data);
            std::copy(byteData, byteData + indexAccessor.count, indices.begin());
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        {
            const auto* shortData = reinterpret_cast<const unsigned short*>(data);
            std::copy(shortData, shortData + indexAccessor.count, indices.begin());
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        {
            const auto* intData = reinterpret_cast<const unsigned int*>(data);
            std::copy(intData, intData + indexAccessor.count, indices.begin());
            break;
        }
        default:
            throw std::runtime_error("Unsupported index component type.");
    }

    return indices;
}

// Updated mesh creation process using safe conversions and component verification
Ref<bee::resource::Mesh> bee::resource::CreateMeshHandle(const tinygltf::Model& model,
                                                         const tinygltf::Primitive& primitive,
                                                         const std::string& name)
{
    // Create mesh
    Ref<bee::resource::Mesh> mesh = bee::resource::CreateResource<bee::resource::Mesh>(name);

    if (mesh->is_valid())
    {
        return mesh;
    }

    // Handle positions
    std::vector<float> positionBuffer;
    if (primitive.attributes.find("POSITION") == primitive.attributes.end())
    {
        bee::Log::Error("No position attribute found in GLTF primitive");
        return nullptr;
    }
    const auto& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
    const auto& posBufferView = model.bufferViews[posAccessor.bufferView];
    const auto* positions = GetAttributeData<float>(posAccessor, model, TINYGLTF_TYPE_VEC3);

    size_t posStride = posBufferView.byteStride == 0 ? sizeof(glm::vec3) : posBufferView.byteStride;

    for (size_t i = 0; i < posAccessor.count; i++)
    {
        const auto* pos = (const float*)((const char*)positions + i * posStride);
        positionBuffer.push_back(pos[0]);
        positionBuffer.push_back(pos[1]);
        positionBuffer.push_back(pos[2]);
    }

    // Handle normals
    std::vector<float> normalBuffer;
    if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
    {
        const auto& normalAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
        const auto& normalBufferView = model.bufferViews[normalAccessor.bufferView];
        const auto* normals = GetAttributeData<float>(normalAccessor, model, TINYGLTF_TYPE_VEC3);

        size_t normalStride = normalBufferView.byteStride == 0 ? sizeof(glm::vec3) : normalBufferView.byteStride;

        for (size_t i = 0; i < normalAccessor.count; i++)
        {
            const auto* norm = (const float*)((const char*)normals + i * normalStride);
            normalBuffer.push_back(norm[0]);
            normalBuffer.push_back(norm[1]);
            normalBuffer.push_back(norm[2]);
        }
    }

    // Handle texture coordinates
    std::vector<float> texCoordBuffer;
    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
    {
        const auto& texAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
        const auto& texBufferView = model.bufferViews[texAccessor.bufferView];
        const auto* texCoords = GetAttributeData<float>(texAccessor, model, TINYGLTF_TYPE_VEC2);

        size_t texStride = texBufferView.byteStride == 0 ? sizeof(glm::vec2) : texBufferView.byteStride;

        for (size_t i = 0; i < texAccessor.count; i++)
        {
            const auto* tex = (const float*)((const char*)texCoords + i * texStride);
            texCoordBuffer.push_back(tex[0]);
            texCoordBuffer.push_back(tex[1]);
        }
    }

    // handle vertex colors
    std::vector<float> colorBuffer;
    if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
    {
        const auto& colorAccessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
        const auto& colorBufferView = model.bufferViews[colorAccessor.bufferView];
        const auto* colors = GetAttributeData<float>(colorAccessor, model, TINYGLTF_TYPE_VEC4);

        size_t colorStride = colorBufferView.byteStride == 0 ? sizeof(glm::vec4) : colorBufferView.byteStride;

        for (size_t i = 0; i < colorAccessor.count; i++)
        {
            const auto* color = (const float*)((const char*)colors + i * colorStride);
            colorBuffer.push_back(color[0]);
            colorBuffer.push_back(color[1]);
            colorBuffer.push_back(color[2]);
        }
    }

    // Handle indices
    const auto& indexAccessor = model.accessors[primitive.indices];
    std::vector<unsigned int> indices = ConvertIndicesToUnsignedInt(indexAccessor, model);

    // Pass corrected buffers to xsr::create_mesh
    mesh->GetHandle() = xsr::create_mesh(indices.data(),
                                               (int)indexAccessor.count,
                                               positionBuffer.data(),
                                               normalBuffer.data(),
                                               texCoordBuffer.data(),
                                               colorBuffer.data(),
                                               (int)posAccessor.count);
    return mesh;
}

// Creates a texture and returns a handle
Ref<bee::resource::Texture> bee::resource::CreateTextureHandle(const tinygltf::Model& model,
                                                               const std::string& name,
                                                               const tinygltf::Primitive& primitive)
{
    Ref<bee::resource::Texture> textureResource;

    // Check if the primitive has a material with a texture
    if (primitive.material >= 0 && primitive.material < (int)model.materials.size())
    {
        const auto& material = model.materials[primitive.material];

        // Check for a base color texture in pbrMetallicRoughness
        if (material.pbrMetallicRoughness.baseColorTexture.index >= 0)
        {
            int textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;

            if (textureIndex >= 0 && textureIndex < (int)model.textures.size())
            {
                const auto& texture = model.textures[textureIndex];

                if (texture.source >= 0 && texture.source < (int)model.images.size())
                {
                    const auto& textureImage = model.images[texture.source];

                    // Set texture name and handle default name if empty
                    std::string textureName = !textureImage.name.empty() ? textureImage.name : textureImage.uri;
                    if (textureName.empty())
                    {
                        textureName = name + "_texture";
                    }

                    textureResource = bee::resource::CreateResource<bee::resource::Texture>(textureName);

                    // Load texture data if it hasn't been initialized yet
                    if (!textureResource->IsValid())
                    {
                        textureResource->GetHandle() =
                            xsr::create_texture(textureImage.width, textureImage.height, textureImage.image.data());
                        textureResource->GetHandle().width = textureImage.width;
                        textureResource->GetHandle().height = textureImage.height;
                    }
                    return textureResource;
                }
            }
        }
    }

    // Use default texture if no valid texture is found
    textureResource = bee::resource::LoadResource<bee::resource::Texture>(
        bee::Engine.FileIO().GetPath(bee::FileIO::Directory::Editor, DEFAULT_TEXTURE));

    return textureResource;
}

// Retrieves the multiplier and tint colors from the GLTF material
std::tuple<glm::vec4, glm::vec4> bee::resource::GetMaterialColors(const tinygltf::Model& model,
                                                                  const tinygltf::Primitive& primitive)
{
    auto multiplier = glm::vec4(1.0f);
    auto tint = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    if (primitive.material >= 0)
    {
        const auto& material = model.materials[primitive.material];

        if (material.pbrMetallicRoughness.baseColorFactor.size() == 4)
        {
            multiplier = glm::vec4(material.pbrMetallicRoughness.baseColorFactor[0],
                                   material.pbrMetallicRoughness.baseColorFactor[1],
                                   material.pbrMetallicRoughness.baseColorFactor[2],
                                   material.pbrMetallicRoughness.baseColorFactor[3]);
        }

        if (material.emissiveFactor.size() == 3)
        {
            tint = glm::vec4(material.emissiveFactor[0],
                             material.emissiveFactor[1],
                             material.emissiveFactor[2],
                             1.0f  // Assuming alpha for tint is always 1.0f
            );
        }
    }

    return {multiplier, tint};
}


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/