#pragma once

#include "runtime/core/vulkan/render_pass.h"
#include <functional>

namespace Bamboo
{
	class UIPass : public RenderPass
	{
	public:
		virtual void init() override;
		virtual void prepare() override;
		virtual void record() override;
		virtual void destroy() override;

		virtual void createRenderPass() override;
		virtual void createDescriptorPool() override;
		virtual void createDescriptorSetLayout() override {}
		virtual void createPipelineLayout() override {}
		virtual void createPipeline() override {}
		virtual void createFramebuffer() override;
		virtual void createResizableObjects(uint32_t width, uint32_t height) override;
		virtual void destroyResizableObjects() override;

		void setConstructFunc(const std::function<void()>& construct_func) { m_construct_func = construct_func; }

	private:
		std::function<void()> m_construct_func;
		std::vector<VkFramebuffer> m_framebuffers;
	};
}