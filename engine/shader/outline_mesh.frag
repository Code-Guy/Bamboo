#version 450
#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

layout(location = 0) in vec3 f_position;
layout(location = 1) in vec2 f_tex_coord;
layout(location = 2) in vec3 f_normal;

layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 1) uniform sampler2D base_color_texture_sampler;

void main()
{
	float alpha = texture(base_color_texture_sampler, f_tex_coord).a;
	if (alpha < MIN_OUTLINE_ALPHA)
	{
		discard;
	}
	o_color = vec4(1.0, 0.0, 0.0, 1.0);
}