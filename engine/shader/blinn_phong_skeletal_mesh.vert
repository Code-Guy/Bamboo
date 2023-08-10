#version 450
#extension GL_GOOGLE_include_directive : enable

#include "host_device.h"

layout(set = 0, binding = 0) uniform _BoneUBO { BoneUBO bone_ubo; };
layout(push_constant) uniform _TransformPCO { TransformPCO transform_pco; };

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
	mat4 blend_bone_matrix = mat4(0.0);
	for (int i = 0; i < BONE_NUM_PER_VERTEX; ++i)
	{
		blend_bone_matrix += bone_ubo.bone_matrices[bones[i]] * weights[i];
	}

	vec4 local_position = blend_bone_matrix * vec4(position, 1.0);
	vec4 local_normal = blend_bone_matrix * vec4(normal, 0.0);
	
	f_position = (transform_pco.m * local_position).xyz;
	f_tex_coord = tex_coord;
	f_normal = (transform_pco.m * local_normal).xyz;

	gl_Position = transform_pco.mvp * local_position;
}