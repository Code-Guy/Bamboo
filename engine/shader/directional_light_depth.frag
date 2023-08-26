#version 450
#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

layout(set = 0, binding = 2) uniform sampler2D base_color_texture_sampler;
layout(location = 0) in vec2 g_tex_coord;

void main() 
{	
	float alpha = texture(base_color_texture_sampler, g_tex_coord).a;
	if (alpha < MIN_SHADOW_ALPHA)
	{
		discard;
	}
}