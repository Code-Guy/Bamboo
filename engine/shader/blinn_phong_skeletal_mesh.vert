#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "host_device.h"

layout(binding = 0) uniform UBO
{
	mat4 gBones[MAX_BONE_NUM];
} ubo;

layout(push_constant) uniform _VertPCO { VertPCO v_pco; };

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in vec3 normal;
layout(location = 3) in ivec4 bones;
layout(location = 4) in vec4 weights;

layout(location = 0) out vec3 f_position;
layout(location = 1) out vec2 f_tex_coord;
layout(location = 2) out vec3 f_normal;

void main()
{
	mat4 bone_transform = mat4(0.0);
	for (int i = 0; i < BONE_NUM_PER_VERTEX; ++i)
	{
		bone_transform += ubo.gBones[bones[i]] * weights[i];
	}

	vec4 local_position = bone_transform * vec4(position, 1.0);
	vec4 local_normal = bone_transform * vec4(normal, 0.0);

	gl_Position = v_pco.mvp * local_position;
	
	f_position = (v_pco.m * local_position).xyz;
	f_tex_coord = tex_coord;
	f_normal = (v_pco.m * local_normal).xyz;
}