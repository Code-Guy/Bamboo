#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "shader_structure.h"

layout(push_constant) uniform FragPCO
{
	layout(offset = 128)
	vec3 camera_pos; float padding0;
	vec3 light_dir; float padding1;
	vec3 base_color_factor; int has_base_color_texture;
	vec3 emissive_factor; int has_emissive_texture;
	float m_metallic_factor;
	float m_roughness_factor;
} f_pco;

layout(binding = 1) uniform sampler2D base_color_texture_sampler;

layout(location = 0) in vec2 f_tex_coord;
layout(location = 1) in vec3 f_normal;
layout(location = 2) in vec3 f_position;

layout(location = 0) out vec4 f_color;

void main()
{
	vec3 base_color = bool(f_pco.has_base_color_texture) ? texture(base_color_texture_sampler, f_tex_coord).rgb : f_pco.base_color_factor;

	// ambient
	float ambient = 0.05;

	// diffuse
	float diffuse = max(dot(-f_pco.light_dir, f_normal), 0.0);

	// specular
	float shininess = 64.0;
	vec3 light_color = vec3(0.5);

	vec3 view_dir = normalize(f_pco.camera_pos - f_position);
	vec3 reflect_dir = reflect(f_pco.light_dir, f_normal);
	vec3 halfway_dir = normalize(-f_pco.light_dir + f_normal);
	float specular = pow(max(dot(halfway_dir, f_normal), 0.0), shininess);

	f_color = vec4(base_color * ambient + base_color * light_color * diffuse + light_color * specular, 1.0);
}