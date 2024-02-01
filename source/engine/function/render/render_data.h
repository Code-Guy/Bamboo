#pragma once

#include "engine/core/vulkan/vulkan_util.h"
#include "host_device.h"

namespace Bamboo
{
	enum class ERenderDataType
	{
		Base, Lighting, StaticMesh, SkeletalMesh, Skybox, Billboard, PostProcess
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
		ERenderDataType type = ERenderDataType::Base;
	};

	struct LightingRenderData : public RenderData
	{
		LightingRenderData() { type = ERenderDataType::Lighting; }

		glm::mat4 camera_view_proj;

		VmaBuffer lighting_ub;

		VmaImageViewSampler irradiance_texture;
		VmaImageViewSampler prefilter_texture;
		VmaImageViewSampler brdf_lut_texture;

		VmaImageViewSampler directional_light_shadow_texture;
		std::vector<VmaImageViewSampler> point_light_shadow_textures;
		std::vector<VmaImageViewSampler> spot_light_shadow_textures;
	};

	struct MeshRenderData : public RenderData
	{
		VmaBuffer vertex_buffer;
		VmaBuffer index_buffer;
		VmaBuffer transform_ub;

		std::vector<uint32_t> index_counts;
		std::vector<uint32_t> index_offsets;
	};

	struct StaticMeshRenderData : public MeshRenderData
	{
		StaticMeshRenderData() { type = ERenderDataType::StaticMesh; }

		std::vector<MaterialPCO> material_pcos;
		std::vector<PBRTexture> pbr_textures;
	};

	struct SkeletalMeshRenderData : public StaticMeshRenderData
	{
		SkeletalMeshRenderData() { type = ERenderDataType::SkeletalMesh; }

		VmaBuffer bone_ub;
	};

	struct SkyboxRenderData : public RenderData
	{
		SkyboxRenderData() { type = ERenderDataType::Skybox; }

		VmaBuffer vertex_buffer;
		VmaBuffer index_buffer;
		uint32_t index_count;

		mat4 mvp;
		VmaImageViewSampler env_texture;
	};

	struct BillboardRenderData : public RenderData
	{
		BillboardRenderData() { type = ERenderDataType::Billboard; }

		vec4 position;
		vec2 size;
		VmaImageViewSampler texture;
	};

	struct PostProcessRenderData : public RenderData
	{
		PostProcessRenderData() { type = ERenderDataType::PostProcess; }

		const VmaImageViewSampler* p_color_texture;
		const VmaImageViewSampler* outline_texture;
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

	struct ShadowFrustumCreateInfo
	{
		vec3 light_pos;
		vec3 light_dir;
		float light_angle;
		float light_near;
		float light_far;
	};
}