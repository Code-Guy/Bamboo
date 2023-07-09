#pragma once

#include "render_pass.h"
#include <functional>

namespace Bamboo
{
	class UIPass : public RenderPass
	{
	public:
		virtual void init() override;
		virtual void prepare() override;
		virtual void render(VkCommandBuffer command_buffer, uint32_t flight_index) override;
		virtual void destroy() override;

		virtual void createRenderPass() override;
		void createDescriptorPool();
		virtual void createDescriptorSetLayouts() override {}
		virtual void createPipelineLayouts() override {}
		virtual void createPipelines() override {}
		virtual void createFramebuffer() override;
		virtual void createResizableObjects(uint32_t width, uint32_t height) override;
		virtual void destroyResizableObjects() override;

		void setConstructFunc(const std::function<void()>& construct_func) { m_construct_func = construct_func; }

	private:
		std::function<void()> m_construct_func;
	};
}