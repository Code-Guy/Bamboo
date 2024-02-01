#version 450
#extension GL_GOOGLE_include_directive : enable

#include "host_device.h"

layout(set = 0, binding = 12) uniform _TransformUBO { TransformUBO transform_ubo; };

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec3 f_position;
layout(location = 1) out vec2 f_tex_coord;
layout(location = 2) out vec3 f_normal;

void main()
{	
	f_position = (transform_ubo.m * vec4(position, 1.0)).xyz;
	f_tex_coord = tex_coord;
	f_normal = normalize(mat3(transform_ubo.nm) * normal);

	gl_Position = transform_ubo.mvp * vec4(position, 1.0);
}