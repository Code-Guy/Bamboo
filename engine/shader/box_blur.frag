#version 450
#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

layout(location = 0) in vec2 f_tex_coord;
layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 0) uniform sampler2D color_texture_sampler;

void main()
{
	ivec2 texture_size = textureSize(color_texture_sampler, 0).xy;
	vec2 texel_size = vec2(1.0 / texture_size.x, 1.0 / texture_size.y);

	o_color = vec4(0.0);
	int samples = 2 * OUTLINE_THICKNESS + 1;
	for (int x = 0; x < samples; ++x)
	{
		for (int y = 0; y < samples; ++y)
		{
			vec2 offset = vec2(x - OUTLINE_THICKNESS, y - OUTLINE_THICKNESS);
	    	o_color += texture(color_texture_sampler, f_tex_coord + offset * texel_size);
		}
	}

	o_color /= samples * samples;
	float alpha = texture(color_texture_sampler, f_tex_coord).a;
	o_color.a = alpha > 0.5 ? 1.0 : 0.5;
}
