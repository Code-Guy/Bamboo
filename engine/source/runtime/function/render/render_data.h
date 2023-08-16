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
		// empty content for base render data
	};

	struct LightingRenderData : public RenderData
	{
		std::vector<VmaBuffer> lighting_ubs;

		VmaImageViewSampler m_irradiance_texture;
		VmaImageViewSampler m_prefilter_texture;
		VmaImageViewSampler m_brdf_lut_texture;
	};

	struct MeshRenderData : public RenderData
	{
		EMeshType mesh_type;
		VmaBuffer vertex_buffer;
		VmaBuffer index_buffer;
		std::vector<uint32_t> index_counts;
		std::vector<uint32_t> index_offsets;
	};

	struct StaticMeshRenderData : public MeshRenderData
	{
		TransformPCO transform_pco;
		std::vector<MaterialPCO> material_pcos;
		std::vector<PBRTexture> pbr_textures;
	};

	struct SkeletalMeshRenderData : public StaticMeshRenderData
	{
		std::vector<VmaBuffer> bone_ubs;
	};
}