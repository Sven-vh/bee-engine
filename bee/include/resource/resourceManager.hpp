#pragma once

#include "common.hpp"
#include "resource/resource.hpp"
#include "resource/mesh.hpp"
#include "resource/texture.hpp"
#include "resource/gltfModel.hpp"

namespace bee::resource
{

// Load a resource from disk
template <typename ResourceType>
Ref<ResourceType> LoadResource(const fs::path& path);

// Create a resource without loading it from disk
template <typename ResourceType>
Ref<ResourceType> CreateResource(const std::string& name);

void OnImGuiRender();

void UnloadResources();

}  // namespace bee::resource