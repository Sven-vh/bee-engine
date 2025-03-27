#pragma once

#include "resource/resource.hpp"
#include "xsr/include/xsr.hpp"

namespace bee::resource
{
class Mesh : public Resource
{
public:
    ~Mesh();

    virtual bool Load(const fs::path& path) override;
    virtual void Unload() override;

    const char* ToString() const override;
    void OnImGuiRender() override;

    xsr::mesh_handle& GetHandle() { return m_handle; }
    const xsr::mesh_handle& GetHandle() const { return m_handle; }

    bool is_valid() const { return m_handle.is_valid(); }

private:
    xsr::mesh_handle m_handle;
    bool m_isRendered = false;
};

}  // namespace bee::resource


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/