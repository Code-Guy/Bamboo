#pragma once

#include "runtime/core/vulkan/vulkan_util.h"
#include <glm/glm.hpp>

namespace Bamboo
{
	struct VertPCO
	{
		glm::mat4 m;
		glm::mat4 mvp;
	};

	struct FragPCO
	{
		glm::vec3 camera_pos; float padding0;
		glm::vec3 light_dir; float padding1;
	};

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