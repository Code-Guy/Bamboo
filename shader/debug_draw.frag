#version 450
#extension GL_GOOGLE_include_directive : enable

#include "hdr.h"

layout(location = 0) in vec3 f_color;
layout(location = 0) out vec4 o_color;

void main()
{
	o_color = vec4(f_color, 1.0);

	// gamma corretion
	o_color.xyz = linear_to_srgb(o_color.xyz);
}