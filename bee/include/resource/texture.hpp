#pragma once

#include "resource/resource.hpp"
#include "xsr/include/xsr.hpp"

namespace bee::resource
{
class Texture : public Resource
{
public:
    Texture() = default;
    ~Texture();

    bool Load(const fs::path& path) override;
    void Unload() override;

    const char* ToString() const override;
    void OnImGuiRender() override;

    xsr::texture_handle& GetHandle() { return m_handle; }
    const xsr::texture_handle& GetHandle() const { return m_handle; }

    bool IsValid() const { return m_handle.is_valid(); }

private:
    xsr::texture_handle m_handle;
};
}  // namespace bee::resource


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/