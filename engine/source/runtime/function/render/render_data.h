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
		std::vector<VkDescriptorSet> ubo_desc_sets;
		std::vector<VkDescriptorSet> texture_desc_sets;

		VertPCO vert_pco;
		FragPCO frag_pco;
	};

	struct RenderDataID
	{
		VmaBuffer vertex_buffer;
		VmaBuffer index_buffer;
		size_t sub_index = 0;

		bool operator<(const RenderDataID& other) const
		{
			return vertex_buffer.buffer < other.vertex_buffer.buffer ||
				(vertex_buffer.buffer == other.vertex_buffer.buffer &&
					index_buffer.buffer < other.index_buffer.buffer) ||
				(vertex_buffer.buffer == other.vertex_buffer.buffer &&
					index_buffer.buffer == other.index_buffer.buffer &&
					sub_index < other.sub_index);
		}
	};


}