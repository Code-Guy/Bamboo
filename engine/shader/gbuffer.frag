#version 450
#extension GL_GOOGLE_include_directive : enable

#include "host_device.h"

layout(push_constant) uniform _MaterialPCO { layout(offset = 192) MaterialPCO material_pco; };

layout(set = 0, binding = 1) uniform sampler2D base_color_texture_sampler;
layout(set = 0, binding = 2) uniform sampler2D metallic_roughness_texture_sampler;
layout(set = 0, binding = 3) uniform sampler2D normal_texture_sampler;
layout(set = 0, binding = 4) uniform sampler2D occlusion_texture_sampler;
layout(set = 0, binding = 5) uniform sampler2D emissive_texture_sampler;

layout(location = 0) in vec3 f_position;
layout(location = 1) in vec2 f_tex_coord;
layout(location = 2) in vec3 f_normal;

layout(location = 0) out vec4 o_normal;
layout(location = 1) out vec4 o_base_color;
layout(location = 2) out vec4 o_emissive_color;
layout(location = 3) out vec4 o_metallic_roughness_occlusion;

vec3 calc_normal()
{
	if (!bool(material_pco.has_normal_texture))
	{
		return f_normal;
	}

	// Perturb normal, see http://www.thetenthplanet.de/archives/1180
	vec3 tangent_normal = texture(normal_texture_sampler, f_tex_coord).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(f_position);
	vec3 q2 = dFdy(f_position);
	vec2 st1 = dFdx(f_tex_coord);
	vec2 st2 = dFdy(f_tex_coord);

	vec3 N = normalize(f_normal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangent_normal);
}

void main()
{
	// normal
	o_normal = vec4(calc_normal(), 0.0);

	// base color
	o_base_color = material_pco.base_color_factor;
	if (bool(material_pco.has_base_color_texture))
	{
		o_base_color *= texture(base_color_texture_sampler, f_tex_coord);
	}

	// emissive color
	o_emissive_color = material_pco.emissive_factor;
	if (bool(material_pco.has_emissive_texture))
	{
		o_emissive_color *= texture(emissive_texture_sampler, f_tex_coord);
	}

	// metallic_roughness_occlusion
	o_metallic_roughness_occlusion.xyz = vec3(material_pco.m_metallic_factor, material_pco.m_roughness_factor, 1.0);
	if (bool(material_pco.has_metallic_roughness_texture))
	{
		vec4 pack_params = texture(metallic_roughness_texture_sampler, f_tex_coord);
		o_metallic_roughness_occlusion.xy *= vec2(pack_params.b, pack_params.g);
	}
	if (bool(material_pco.has_occlusion_texture))
	{
		o_metallic_roughness_occlusion.z *= texture(occlusion_texture_sampler, f_tex_coord).r;
	}
}