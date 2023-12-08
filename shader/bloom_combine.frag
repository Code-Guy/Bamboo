#version 450
#extension GL_GOOGLE_include_directive : enable

#include "hdr.h"

layout(location = 0) in vec2 f_tex_coord;

layout(input_attachment_index = 0, binding = 0) uniform subpassInput blurred_sampler;
layout(set = 0, binding = 1) uniform sampler2D color_texture_sampler;

layout(push_constant) uniform PCO 
{
	float intensity;
} pco;

layout(location = 0) out vec4 o_color;

void main()
{
    vec3 hdr_color = texture(color_texture_sampler, f_tex_coord).rgb;
    vec3 bloom_color = subpassLoad(blurred_sampler).rgb * pco.intensity;
    hdr_color += bloom_color;
    o_color = vec4(hdr_color, 1.0);
}