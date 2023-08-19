#version 450
#extension GL_GOOGLE_include_directive : enable

#include "pbr.h"

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput normal_texture_sampler;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput base_color_texture_sampler;
layout(input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput emissive_color_texture_sampler;
layout(input_attachment_index = 3, set = 0, binding = 3) uniform subpassInput metallic_roughness_occlusion_texture_sampler;
layout(input_attachment_index = 4, set = 0, binding = 4) uniform subpassInput depth_stencil_texture_sampler;

layout(location = 0) in vec2 f_tex_coord;
layout(location = 0) out vec4 o_color;

void main()
{
	float depth = subpassLoad(depth_stencil_texture_sampler).x;
	if (is_nearly_equal(depth, 1.0))
	{
		discard;
	}

	// reconstruct position from depth image
	MaterialInfo mat_info;
    vec4 ndc_pos = vec4(f_tex_coord * 2.0 - 1.0, depth, 1.0);
    vec4 world_position = lighting_ubo.inv_view_proj * ndc_pos;
	mat_info.position = world_position.xyz / world_position.w;
	mat_info.normal = subpassLoad(normal_texture_sampler).xyz;
	
	// pbr material properties
	mat_info.base_color = subpassLoad(base_color_texture_sampler);
	mat_info.emissive_color = subpassLoad(emissive_color_texture_sampler);
	vec3 metallic_roughness_occlusion = subpassLoad(metallic_roughness_occlusion_texture_sampler).xyz;
	mat_info.metallic = metallic_roughness_occlusion.x;
	mat_info.roughness = metallic_roughness_occlusion.y;
	mat_info.occlusion = metallic_roughness_occlusion.z;

	o_color = calc_pbr(mat_info);
}