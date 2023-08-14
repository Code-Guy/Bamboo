#version 450
#extension GL_GOOGLE_include_directive : enable

#include "host_device.h"

layout(push_constant) uniform _MaterialPCO { layout(offset = 192) MaterialPCO material_pco; };

layout(set = 0, binding = 1) uniform sampler2D base_color_texture_sampler;
layout(set = 0, binding = 2) uniform sampler2D metallic_roughness_texture_sampler;
layout(set = 0, binding = 3) uniform sampler2D normal_texture_sampler;
layout(set = 0, binding = 4) uniform sampler2D occlusion_texture_sampler;
layout(set = 0, binding = 5) uniform sampler2D emissive_texture_sampler;
layout(set = 0, binding = 6) uniform _LightingUBO { LightingUBO lighting_ubo; };

layout(location = 0) in vec3 f_position;
layout(location = 1) in vec2 f_tex_coord;
layout(location = 2) in vec3 f_normal;

layout(location = 0) out vec4 o_color;

void main()
{
	vec3 base_color = bool(material_pco.has_base_color_texture) ? 
		texture(base_color_texture_sampler, f_tex_coord).xyz : material_pco.base_color_factor.xyz;

	// ambient
	float ambient = 0.05;

	// diffuse
	float diffuse = max(dot(-lighting_ubo.light_dir, f_normal), 0.0);

	// specular
	float shininess = 64.0;
	vec3 light_color = vec3(0.5);

	vec3 view_dir = normalize(lighting_ubo.camera_pos - f_position);
	vec3 reflect_dir = reflect(lighting_ubo.light_dir, f_normal);
	vec3 halfway_dir = normalize(-lighting_ubo.light_dir + f_normal);
	float specular = pow(max(dot(halfway_dir, f_normal), 0.0), shininess);

	o_color = vec4(base_color * ambient + base_color * light_color * diffuse + light_color * specular, 1.0);
}