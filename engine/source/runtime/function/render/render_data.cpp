#include "render_data.h"
#include "runtime/core/vulkan/vulkan_rhi.h"

namespace Bamboo
{

	void RenderData::destroy()
	{
		for (VmaBuffer& uniform_buffer : uniform_buffers)
		{
			uniform_buffer.destroy();
		}

		index_buffer.destroy();
		vertex_buffer.destroy();
	}

	void BasicRenderData::destroy()
	{
		for (VmaImageViewSampler& image_view_sampler : image_view_samplers)
		{
			image_view_sampler.destroy();
		}
		RenderData::destroy();
	}

}