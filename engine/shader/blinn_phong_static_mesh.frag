#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "shader_structure.h"

layout(push_constant) uniform FPCO
{
	layout(offset = 128)
	vec3 camera_position; float p0;
	vec3 light_direction; float p1;
} fpco;

layout(binding = 1) uniform sampler2D base_color_texture_sampler;

layout(location = 0) in vec2 f_tex_coord;
layout(location = 1) in vec3 f_normal;
layout(location = 2) in vec3 f_position;

layout(location = 0) out vec4 f_color;

void main()
{
	vec3 base_color = texture(base_color_texture_sampler, f_tex_coord).rgb;

	// ambient
	float ambient = 0.05;

	// diffuse
	float diffuse = max(dot(-fpco.light_direction, f_normal), 0.0);

	// specular
	float shininess = 64.0;
	vec3 light_color = vec3(0.5);

	vec3 view_direction = normalize(fpco.camera_position - f_position);
	vec3 reflect_direction = reflect(fpco.light_direction, f_normal);
	vec3 halfway_direction = normalize(-fpco.light_direction + f_normal);
	float specular = pow(max(dot(halfway_direction, f_normal), 0.0), shininess);

	f_color = vec4(base_color * ambient + base_color * light_color * diffuse + light_color * specular, 1.0);
}