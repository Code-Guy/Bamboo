#version 450
#extension GL_GOOGLE_include_directive : enable

#include "hdr.h"

layout(location = 0) in vec3 f_uvw;

layout(binding = 0) uniform samplerCube env_tex;

layout(location = 0) out vec4 o_color;

void main()
{
	const float k_lod = 1.0;
	vec3 color = srgb_to_linear(tonemap(textureLod(env_tex, f_uvw, k_lod).rgb));	
	o_color = vec4(color, 1.0);
}