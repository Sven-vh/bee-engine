#pragma once
#include "rendering/FrameBuffer.hpp"

namespace bee {
	class OpenGLFrameBuffer : public FrameBuffer {
	public:
		OpenGLFrameBuffer(const FrameBufferSettings& settings);
		~OpenGLFrameBuffer();
		void Bind() override;
		void Unbind() override;

		void Clear() override;

		uintptr_t GetColorAttachmentRendererID() const override { return colorAttachment; }

		void Resize(uint32_t width, uint32_t height) override;

		void Invalidate();

		const FrameBufferSettings& GetSettings() const override { return settings; }
	private:
		uint32_t rendererID;
		uint32_t colorAttachment, depthAttachment;
		FrameBufferSettings settings;
	};
}


/*
Read license.txt on root or https://github.com/Sven-vh/bee-engine/blob/main/license.txt
*/