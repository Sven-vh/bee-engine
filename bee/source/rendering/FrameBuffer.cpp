#include "rendering/FrameBuffer.hpp"
#include "platform/opengl/OpenGLFrameBuffer.hpp"

using namespace bee;

Ref<FrameBuffer> bee::FrameBuffer::Create(const FrameBufferSettings& settings)
{
#ifdef BEE_PLATFORM_PC
    return CreateRef<OpenGLFrameBuffer>(settings);
#else
    (void)settings;
    bee::Log::Critical("FrameBuffer::Create: Unknown platform!");
    return nullptr;
#endif
}