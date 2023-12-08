#version 450
#extension GL_GOOGLE_include_directive : enable

#include "hdr.h"

layout(location = 0) in vec2 f_tex_coord;

layout(input_attachment_index = 0, binding = 0) uniform subpassInput scene_sampler;

layout(push_constant) uniform PCO {
    layout(offset = 0) float exposure;
    layout(offset = 4) float intensity;
    layout(offset = 8) float threshold;
} pco;

layout(location = 0) out vec4 o_color;

void main()
{
    vec4 in_color = subpassLoad(scene_sampler);
    o_color = vec4(tonemap(in_color.xyz, pco.exposure), in_color.w);
    //o_color = in_color;
}