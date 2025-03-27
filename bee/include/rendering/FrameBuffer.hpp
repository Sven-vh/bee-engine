#pragma once
#include "common.hpp"

namespace bee
{

struct FrameBufferSettings
{
    uint32_t Width, Height;
    uint32_t Samples = 1;

    bool SwapChainTarget = false;

    FrameBufferSettings() = default;
    FrameBufferSettings(uint32_t width, uint32_t height) : Width(width), Height(height) {}
    FrameBufferSettings(uint32_t width, uint32_t height, uint32_t samples) : Width(width), Height(height), Samples(samples) {}
    FrameBufferSettings(uint32_t width, uint32_t height, bool swapChainTarget)
        : Width(width), Height(height), SwapChainTarget(swapChainTarget)
    {
    }
};

class FrameBuffer
{
public:
    virtual ~FrameBuffer() = default;
    virtual void Bind() = 0;
    virtual void Unbind() = 0;

    virtual void Clear() = 0;

    virtual void Resize(uint32_t width, uint32_t height) = 0;

    virtual uintptr_t GetColorAttachmentRendererID() const = 0;

    virtual const FrameBufferSettings& GetSettings() const = 0;

    static Ref<FrameBuffer> Create(const FrameBufferSettings& settings);
};
}  // namespace bee


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/