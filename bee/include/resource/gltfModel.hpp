#pragma once
#include "resource/resource.hpp"
#include "tinygltf/tiny_gltf.h"

namespace bee::resource
{
class GltfModel : public Resource
{
public:
    ~GltfModel();

    bool Load(const fs::path& path) override;
    void Unload() override;

    const char* ToString() const override { return "GltfModel"; }
    void OnImGuiRender() override;

    tinygltf::Model& GetModel() { return model; }
    const tinygltf::Model& GetModel() const { return model; }

private:
    tinygltf::Model model;

    bool m_isRendered = false;

private:
    static bool IsBinaryGLTF(const fs::path& path);
};
}  // namespace bee::resource