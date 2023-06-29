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

	struct BatchRenderData
	{
		VmaBuffer vertex_buffer;
		VmaBuffer index_buffer;
		std::vector<uint32_t> index_counts;

		std::vector<VmaBuffer> uniform_buffers;
		std::vector<VkDescriptorSet> descriptor_sets;
	};

	struct BlinnPhongBatchRenderData : public BatchRenderData
	{
		std::vector<VmaImageViewSampler> image_view_samplers;

		VertPCO vert_pco;
		FragPCO frag_pco;
	};
}