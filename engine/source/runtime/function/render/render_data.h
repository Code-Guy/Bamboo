#pragma once

#include "runtime/core/vulkan/vulkan_util.h"
#include "host_device.h"

namespace Bamboo
{
	struct RenderData
	{
		VmaBuffer vertex_buffer;
		VmaBuffer index_buffer;
		std::vector<uint32_t> index_counts;
		std::vector<uint32_t> index_offsets;
	};

	struct MeshRenderData : public RenderData
	{
		std::vector<VmaBuffer> uniform_buffers;
		std::vector<VmaImageViewSampler> textures;

		VertPCO vert_pco;
		FragPCO frag_pco;
	};
}