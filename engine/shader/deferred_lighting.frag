#version 450
#extension GL_GOOGLE_include_directive : enable

#include "host_device.h"

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput normal_texture_sampler;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput base_color_texture_sampler;
layout(input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput emissive_color_texture_sampler;
layout(input_attachment_index = 3, set = 0, binding = 3) uniform subpassInput metallic_roughness_occlusion_texture_sampler;
layout(input_attachment_index = 4, set = 0, binding = 4) uniform subpassInput depth_stencil_texture_sampler;

layout(set = 0, binding = 5) uniform samplerCube irradiance_texture_sampler;
layout(set = 0, binding = 6) uniform samplerCube prefilter_texture_sampler;
layout(set = 0, binding = 7) uniform samplerCube brdf_lut_texture_sampler;

layout(set = 0, binding = 8) uniform _LightingUBO { LightingUBO lighting_ubo; };

layout(location = 0) in vec2 f_tex_coord;
layout(location = 0) out vec4 o_color;

void main()
{
	// reconstruct position from depth image
	float depth = subpassLoad(depth_stencil_texture_sampler).x;
    vec4 ndc_pos = vec4(f_tex_coord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 world_position = lighting_ubo.inv_view_proj * ndc_pos;
	vec3 position = world_position.xyz / world_position.w;

	vec3 base_color = subpassLoad(base_color_texture_sampler).xyz;
	vec3 normal = subpassLoad(normal_texture_sampler).xyz;

	// get lighting infos
	vec3 light_dir = lighting_ubo.directional_light.direction;
	vec3 light_color = lighting_ubo.directional_light.color;

	// ambient
	float ambient = 0.05;

	// diffuse
	float diffuse = max(dot(-light_dir, normal), 0.0);

	// specular
	float shininess = 64.0;
	vec3 view_dir = normalize(lighting_ubo.camera_pos - position);
	vec3 reflect_dir = reflect(light_dir, normal);
	vec3 halfway_dir = normalize(-light_dir + normal);
	float specular = pow(max(dot(halfway_dir, normal), 0.0), shininess);
	specular = 0.0;

	o_color = vec4(base_color * ambient + base_color * light_color * diffuse + light_color * specular, 1.0);
}