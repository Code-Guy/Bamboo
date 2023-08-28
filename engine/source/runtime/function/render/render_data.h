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
		VmaImageViewSampler metallic_roughness_occlusion_texure;
		VmaImageViewSampler normal_texure;
		VmaImageViewSampler emissive_texure;
	};

	struct RenderData
	{
		// empty content for base render data
	};

	struct LightingRenderData : public RenderData
	{
		std::vector<VmaBuffer> lighting_ubs;

		VmaImageViewSampler irradiance_texture;
		VmaImageViewSampler prefilter_texture;
		VmaImageViewSampler brdf_lut_texture;

		VmaImageViewSampler directional_light_shadow_texture;
		VmaImageViewSampler point_light_shadow_texture;
	};

	struct MeshRenderData : public RenderData
	{
		EMeshType mesh_type;
		VmaBuffer vertex_buffer;
		VmaBuffer index_buffer;
		std::vector<uint32_t> index_counts;
		std::vector<uint32_t> index_offsets;
		TransformPCO transform_pco;
	};

	struct StaticMeshRenderData : public MeshRenderData
	{
		std::vector<MaterialPCO> material_pcos;
		std::vector<PBRTexture> pbr_textures;
	};

	struct SkeletalMeshRenderData : public StaticMeshRenderData
	{
		std::vector<VmaBuffer> bone_ubs;
	};

	struct SkyboxRenderData : public RenderData
	{
		VmaBuffer vertex_buffer;
		VmaBuffer index_buffer;
		uint32_t index_count;
		TransformPCO transform_pco;
		VmaImageViewSampler env_texture;
	};

	struct ShadowCascadeCreateInfo
	{
		float camera_near;
		float camera_far;
		mat4 inv_camera_view_proj;

		vec3 light_dir;
		float light_cascade_frustum_near;
	};

	struct ShadowCubeCreateInfo
	{
		vec3 light_pos;
		float light_near;
		float light_far;
	};
}