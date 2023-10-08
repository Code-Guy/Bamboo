#version 450
#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

layout(set = 0, binding = 1) uniform sampler2D base_color_texture_sampler;

layout(location = 0) in vec3 f_position;
layout(location = 1) in vec2 f_tex_coord;
layout(location = 2) in vec3 f_normal;

void main() 
{	
	float alpha = texture(base_color_texture_sampler, f_tex_coord).a;
	if (alpha < MIN_SHADOW_ALPHA)
	{
		discard;
	}
}