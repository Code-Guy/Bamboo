#pragma once

#include "runtime/core/vulkan/vulkan_util.h"
#include "host_device.h"

namespace Bamboo
{
	enum class EMeshType
	{
		Static, Skeletal
	};

	struct PBRTexture
	{
		VmaImageViewSampler base_color_texure;
		VmaImageViewSampler metallic_roughness_texure;
		VmaImageViewSampler normal_texure;
		VmaImageViewSampler occlusion_texure;
		VmaImageViewSampler emissive_texure;
	};

	struct RenderData
	{

	};

	struct DeferredLightingRenderData : public RenderData
	{
		std::vector<VmaBuffer> lighting_ubs;
	};

	struct MeshRenderData : public RenderData
	{
		VmaBuffer vertex_buffer;
		VmaBuffer index_buffer;
		std::vector<uint32_t> index_counts;
		std::vector<uint32_t> index_offsets;
	};

	struct StaticMeshRenderData : public MeshRenderData
	{
		EMeshType mesh_type;

		std::vector<VmaBuffer> lighting_ubs;
		std::vector<PBRTexture> pbr_textures;

		TransformPCO transform_pco;
		std::vector<MaterialPCO> material_pcos;
	};

	struct SkeletalMeshRenderData : public StaticMeshRenderData
	{
		std::vector<VmaBuffer> bone_ubs;
	};
}