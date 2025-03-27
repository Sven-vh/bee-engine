#include "resource/texture.hpp"
#include "core/fileio.hpp"
#include "core/engine.hpp"
#include "core.hpp"

bee::resource::Texture::~Texture() { Unload(); }

bool bee::resource::Texture::Load(const fs::path& path)
{
    std::vector<char> textureData = bee::Engine.FileIO().ReadBinaryFile(bee::FileIO::Directory::None, path);
    if (textureData.empty())
    {
        m_handle = xsr::texture_handle();
        return false;
    }
    m_handle = xsr::tools::load_png_texture(textureData);

    return m_handle.is_valid();
}

void bee::resource::Texture::Unload()
{
    xsr::unload_texture(m_handle);
    m_handle = xsr::texture_handle();
}

const char* bee::resource::Texture::ToString() const { return "Texture"; }

void bee::resource::Texture::OnImGuiRender()
{
    if (m_handle.is_valid())
    {
        void* texture_id = xsr::get_texture_id(m_handle);
        if (texture_id)
        {
            ImVec2 final_size = bee::ImGuiHelper::GetMaxImageSize((float)m_handle.width, (float)m_handle.height);
            ImGui::Image(texture_id, final_size);
        }
    }
}



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/