#version 450
#extension GL_GOOGLE_include_directive : enable

#include "host_device.h"

layout(set = 0, binding = 0) uniform sampler2D position_texture_sampler;
layout(set = 0, binding = 1) uniform sampler2D normal_texture_sampler;
layout(set = 0, binding = 2) uniform sampler2D base_color_texture_sampler;
layout(set = 0, binding = 3) uniform sampler2D emissive_color_texture_sampler;
layout(set = 0, binding = 4) uniform sampler2D metallic_roughness_occlusion_texture_sampler;

layout(set = 0, binding = 5) uniform _LightingUBO { LightingUBO lighting_ubo; };

layout(location = 0) in vec2 f_tex_coord;
layout(location = 0) out vec4 o_color;

void main() 
{
	vec3 position = texture(position_texture_sampler, f_tex_coord).xyz;
	vec3 base_color = texture(base_color_texture_sampler, f_tex_coord).xyz;
	vec3 normal = texture(normal_texture_sampler, f_tex_coord).xyz;

	// ambient
	float ambient = 0.05;

	// diffuse
	float diffuse = max(dot(-lighting_ubo.light_dir, normal), 0.0);

	// specular
	float shininess = 64.0;
	vec3 light_color = vec3(0.5);

	vec3 view_dir = normalize(lighting_ubo.camera_pos - position);
	vec3 reflect_dir = reflect(lighting_ubo.light_dir, normal);
	vec3 halfway_dir = normalize(-lighting_ubo.light_dir + normal);
	float specular = pow(max(dot(halfway_dir, normal), 0.0), shininess);

	o_color = vec4(base_color * ambient + base_color * light_color * diffuse + light_color * specular, 1.0);	
}