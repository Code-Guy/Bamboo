#version 450
#extension GL_GOOGLE_include_directive : enable

#include "constants.h"

layout(push_constant) uniform PCO { 
    vec4 light_pos;
} pco;

layout(binding = 3) uniform sampler2D base_color_texture_sampler;

layout(location = 0) in vec3 g_position;
layout(location = 1) in vec2 g_tex_coord;

layout(location = 0) out float o_depth;

void main() 
{   
    float alpha = texture(base_color_texture_sampler, g_tex_coord).a;
    if (alpha < MIN_SHADOW_ALPHA)
    {
        discard;
    }

    o_depth = length(pco.light_pos.xyz - g_position);
}