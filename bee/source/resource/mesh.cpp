#include "resource/mesh.hpp"
#include "core/fileio.hpp"
#include "core/engine.hpp"
#include "core.hpp"

bee::resource::Mesh::~Mesh() { Unload(); }

bool bee::resource::Mesh::Load(const fs::path& path)
{
    std::string data = bee::Engine.FileIO().ReadTextFile(bee::FileIO::Directory::None, path);
    if (data.empty())
    {
        m_handle = xsr::mesh_handle();
        return false;
    }
    m_handle = xsr::tools::load_obj_mesh(data);
    return m_handle.is_valid();
}

void bee::resource::Mesh::Unload()
{
    xsr::unload_mesh(m_handle);
    m_handle = xsr::mesh_handle();
}

const char* bee::resource::Mesh::ToString() const { return "Mesh"; }

void bee::resource::Mesh::OnImGuiRender()
{
    FrameBufferSettings settings(ICON_SIZE, ICON_SIZE);
    static Ref<bee::FrameBuffer> frameBuffer = bee::FrameBuffer::Create(settings);
    //if (!m_isRendered)
    //{
        entt::entity newEntity = bee::ecs::CreateMesh();
        bee::Renderable& renderable = bee::Engine.Registry().get<bee::Renderable>(newEntity);
        renderable.mesh->GetHandle() = m_handle;
        bee::ecs::RenderEntity(bee::Engine.Registry(), newEntity, frameBuffer);
        bee::ecs::DestroyEntity(newEntity, bee::Engine.Registry());

        m_isRendered = true;
    //}
    ImVec2 size = bee::ImGuiHelper::GetMaxImageSize((float)settings.Width, (float)settings.Height);
    ImGui::Image((void*)frameBuffer->GetColorAttachmentRendererID(), size, ImVec2(0, 1), ImVec2(1, 0));
}
