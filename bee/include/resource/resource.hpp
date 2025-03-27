#pragma once
#include "common.hpp"

#define ICON_SIZE 256

namespace bee::resource
{
class Resource
{
public:
    virtual ~Resource() = default;

    virtual bool Load(const fs::path& path) = 0;
    virtual void Unload() = 0;

    virtual const char* ToString() const { return "Resource"; }
    virtual void OnImGuiRender() {}
};
}  // namespace bee::resource


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/