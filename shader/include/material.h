#ifndef MATERIAL
#define MATERIAL

#include "host_device.h"

layout(push_constant) uniform _MaterialPCO { MaterialPCO material_pco; };

layout(binding = 2) uniform sampler2D base_color_texture_sampler;
layout(binding = 3) uniform sampler2D metallic_roughness_occlusion_texture_sampler;
layout(binding = 4) uniform sampler2D normal_texture_sampler;
layout(binding = 5) uniform sampler2D emissive_texture_sampler;

layout(location = 0) in vec3 f_position;
layout(location = 1) in vec2 f_tex_coord;
layout(location = 2) in vec3 f_normal;

vec3 calc_normal()
{
	if (!bool(material_pco.has_normal_texture))
	{
		return f_normal;
	}

	// Perturb normal, see http://www.thetenthplanet.de/archives/1180
	vec4 texel = texture(normal_texture_sampler, f_tex_coord);
	vec3 tangent_normal = texel.xyz;
	if (texel.w > 0.999)
	{
		tangent_normal = tangent_normal * 2.0 - 1.0;
	}
	else
	{
		tangent_normal.xy = texel.xw * 2.0 - 1.0;
		tangent_normal.z = sqrt(1 - dot(tangent_normal.xy, tangent_normal.xy));
	}

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

MaterialInfo calc_material_info()
{
	MaterialInfo mat_info;

	// position
	mat_info.position = f_position;

	// normal
	mat_info.normal = calc_normal();

	// base color
	mat_info.base_color = material_pco.base_color_factor;
	if (bool(material_pco.has_base_color_texture))
	{
		mat_info.base_color *= texture(base_color_texture_sampler, f_tex_coord);
	}

	// emissive color
	mat_info.emissive_color = material_pco.emissive_factor;
	if (bool(material_pco.has_emissive_texture))
	{
		mat_info.emissive_color = texture(emissive_texture_sampler, f_tex_coord);
	}

	// metallic_roughness_occlusion
	vec3 metallic_roughness_occlusion = vec3(material_pco.m_metallic_factor, material_pco.m_roughness_factor, 1.0);
	if (bool(material_pco.has_metallic_roughness_occlusion_texture))
	{
		vec4 pack_params = texture(metallic_roughness_occlusion_texture_sampler, f_tex_coord);
		metallic_roughness_occlusion.xyz *= vec3(pack_params.b, pack_params.g, bool(material_pco.contains_occlusion_channel) ? pack_params.r : 1.0);
	}
	mat_info.metallic = metallic_roughness_occlusion.x;
	mat_info.roughness = metallic_roughness_occlusion.y;
	mat_info.occlusion = metallic_roughness_occlusion.z;

	return mat_info;
}

#endif