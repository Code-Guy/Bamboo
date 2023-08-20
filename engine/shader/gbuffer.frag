#version 450
#extension GL_GOOGLE_include_directive : enable

#include "material.h"

layout(location = 0) out vec4 o_normal;
layout(location = 1) out vec4 o_base_color;
layout(location = 2) out vec4 o_emissive_color;
layout(location = 3) out vec4 o_metallic_roughness_occlusion;

void main()
{
	MaterialInfo mat_info = calc_material_info();

	// gbuffers
	o_normal = vec4(mat_info.normal, 0.0);
	o_base_color = mat_info.base_color;
	o_emissive_color = mat_info.emissive_color;
	o_metallic_roughness_occlusion.xyz = vec3(mat_info.metallic, mat_info.roughness, mat_info.occlusion);
}