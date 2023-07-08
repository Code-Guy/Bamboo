#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "shader_structure.h"

layout(binding = 0) uniform UBO
{
	mat4 padding;
} ubo;

layout(push_constant) uniform VertPCO
{
	mat4 m;
	mat4 mvp;
} v_pco;


layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec2 f_tex_coord;
layout(location = 1) out vec3 f_normal;
layout(location = 2) out vec3 f_position;

void main()
{	
	f_tex_coord = tex_coord;
	f_normal = (v_pco.m * vec4(normal, 0.0)).xyz;
	f_position = (v_pco.m * vec4(position, 1.0)).xyz;

	gl_Position = v_pco.mvp * vec4(position, 1.0);
}