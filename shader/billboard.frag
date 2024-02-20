#version 450
#extension GL_GOOGLE_include_directive : enable

#include "hdr.h"

layout(location = 0) in vec2 g_tex_coord;

layout(binding = 0) uniform sampler2D texture_sampler;

layout(location = 0) out vec4 o_color;

void main()
{
	o_color = texture(texture_sampler, g_tex_coord);

	// gamma corretion
	o_color.xyz = linear_to_srgb(o_color.xyz);
}