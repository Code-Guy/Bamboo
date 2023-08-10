#pragma once

#include "runtime/core/vulkan/vulkan_util.h"
#include "host_device.h"

namespace Bamboo
{
	enum class EMeshType
	{
		Static, Skeletal
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
		EMeshType mesh_type;

		std::vector<VmaBuffer> lighting_ubs;
		std::vector<VmaImageViewSampler> textures;

		TransformPCO transform_pco;
		std::vector<MaterialPCO> material_pcos;
	};

	struct SkeletalRenderData : public MeshRenderData
	{
		std::vector<VmaBuffer> bone_ubs;
	};
}