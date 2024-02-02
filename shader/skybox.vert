#version 450
#extension GL_GOOGLE_include_directive : enable

#include "host_device.h"

layout(push_constant) uniform PCO 
{
	mat4 mvp;
} pco;

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coord;
layout(location = 2) in vec3 normal;

layout (location = 0) out vec3 f_uvw;

void main()
{	
	f_uvw = position.xyz;

	vec4 pos = pco.mvp * vec4(position, 1.0);
	gl_Position = pos.xyww;
}