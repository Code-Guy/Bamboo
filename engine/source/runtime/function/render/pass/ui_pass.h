#pragma once

#include "render_pass.h"
#include <functional>

namespace Bamboo
{
	class UIPass : public RenderPass
	{
	public:
		virtual void init() override;
		void prepare();
		virtual void render() override;
		virtual void destroy() override;

		virtual void createRenderPass() override;
		void createDescriptorPool();
		virtual void createDescriptorSetLayouts() override {}
		virtual void createPipelineLayouts() override {}
		virtual void createPipelines() override {}
		virtual void createFramebuffer() override;
		virtual void destroyResizableObjects() override;

	private:
		std::vector<VkFramebuffer> m_framebuffers;
	};
}