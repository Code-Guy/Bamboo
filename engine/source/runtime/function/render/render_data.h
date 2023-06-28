#pragma once

#include "runtime/core/vulkan/vulkan_util.h"
#include <glm/glm.hpp>

namespace Bamboo
{
	struct VertexPCO
	{
		glm::mat4 m;
		glm::mat4 mvp;
	};

	struct FragmentPCO
	{
		glm::vec3 camera_pos; float padding0;
		glm::vec3 light_dir; float padding1;
	};

	struct RenderData
	{
		VmaBuffer vertex_buffer;
		VmaBuffer index_buffer;
		std::vector<uint32_t> index_counts;

		std::vector<VmaBuffer> uniform_buffers;
		std::vector<VkDescriptorSet> descriptor_sets;

		virtual void destroy();
	};

	struct BasicRenderData : public RenderData
	{
		std::vector<VmaImageViewSampler> image_view_samplers;
		VertexPCO vertex_pco;
		FragmentPCO fragment_pco;

		virtual void destroy();
	};
}