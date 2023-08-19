#version 450
#extension GL_GOOGLE_include_directive : enable

#include "pbr.h"
#include "material.h"

layout(set = 0, binding = 9) uniform sampler2D emissive_texture_sampler;

layout(location = 0) out vec4 o_color;

void main()
{
	MaterialInfo mat_info = calc_material_info(emissive_texture_sampler);
	o_color = calc_pbr(mat_info);
	o_color.a = 0.5;
}