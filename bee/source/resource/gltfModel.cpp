#include "resource/gltfModel.hpp"
#include "core.hpp"

bee::resource::GltfModel::~GltfModel() { Unload(); }

bool bee::resource::GltfModel::IsBinaryGLTF(const fs::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        std::cerr << "Failed to open file: " << path << std::endl;
        return false;
    }

    // Read the first 4 bytes
    char header[4];
    file.read(header, 4);

    // Check if it's the magic string 'glTF'
    return (header[0] == 'g' && header[1] == 'l' && header[2] == 'T' && header[3] == 'F');
}

bool bee::resource::GltfModel::Load(const fs::path& path)
{
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool isBinary = IsBinaryGLTF(path);

    bool exists = std::filesystem::exists(path);
    if (!exists)
    {
        bee::Log::Error("File does not exist: {}", path.string());
        return false;
    }

    bool success = false;
    if (isBinary)
    {
        success = loader.LoadBinaryFromFile(&model, &err, &warn, path.string());
    }
    else
    {
        success = loader.LoadASCIIFromFile(&model, &err, &warn, path.string());
    }
    LogGLTFWarningsAndErrors(warn, err, path);

    return success;
}

void bee::resource::GltfModel::Unload() { model = tinygltf::Model(); }

void bee::resource::GltfModel::OnImGuiRender()
{
    FrameBufferSettings settings(ICON_SIZE, ICON_SIZE);
    static Ref<bee::FrameBuffer> frameBuffer = bee::FrameBuffer::Create(settings);
    //if (!m_isRendered)
    //{
        entt::entity newEntity = bee::resource::LoadGLTFFromModel(bee::Engine.Registry(), model);
        bee::ecs::RenderEntity(bee::Engine.Registry(), newEntity, frameBuffer);
        bee::ecs::DestroyEntity(newEntity, bee::Engine.Registry());

        m_isRendered = true;
    //}
    ImVec2 size = bee::ImGuiHelper::GetMaxImageSize((float)settings.Width, (float)settings.Height);
    ImGui::Image((void*)frameBuffer->GetColorAttachmentRendererID(), size, ImVec2(0, 1), ImVec2(1, 0));
}
