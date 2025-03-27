#include "platform/opengl/OpenGLFrameBuffer.hpp"
#include "glad/glad.h"
#include <cassert>

#define assert_msg(x, msg)       \
    if (!(x))                    \
    {                            \
        bee::Log::Critical(msg); \
        __debugbreak();          \
    }

bee::OpenGLFrameBuffer::OpenGLFrameBuffer(const FrameBufferSettings& settings) : settings(settings) { Invalidate(); }

bee::OpenGLFrameBuffer::~OpenGLFrameBuffer()
{
    glDeleteFramebuffers(1, &rendererID);
    glDeleteTextures(1, &colorAttachment);
    glDeleteTextures(1, &depthAttachment);
}

void bee::OpenGLFrameBuffer::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, rendererID);
    glViewport(0, 0, settings.Width, settings.Height);
}

void bee::OpenGLFrameBuffer::Unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void bee::OpenGLFrameBuffer::Clear() { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }

void bee::OpenGLFrameBuffer::Resize(uint32_t width, uint32_t height)
{
    settings.Width = width;
    settings.Height = height;

    Invalidate();
}

void bee::OpenGLFrameBuffer::Invalidate()
{
    if (rendererID)
    {
        glDeleteFramebuffers(1, &rendererID);
        glDeleteTextures(1, &colorAttachment);
        glDeleteTextures(1, &depthAttachment);
    }

    glCreateFramebuffers(1, &rendererID);
    glBindFramebuffer(GL_FRAMEBUFFER, rendererID);

    glCreateTextures(GL_TEXTURE_2D, 1, &colorAttachment);
    glBindTexture(GL_TEXTURE_2D, colorAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, settings.Width, settings.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTextureParameteri(colorAttachment, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(colorAttachment, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorAttachment, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &depthAttachment);
    glBindTexture(GL_TEXTURE_2D, depthAttachment);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, settings.Width, settings.Height);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthAttachment, 0);

    assert_msg(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/