#version 450

layout(location = 0) in vec2 f_tex_coord;
layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform sampler2D color_texture_sampler;
layout(binding = 1) uniform sampler2D outline_texture_sampler;
layout(binding = 2) uniform sampler3D color_grading_lut_texture_sampler;

layout(push_constant) uniform PCO 
{
	int is_selecting;
} pco;

#define OUTLINE_COLOR vec4(0.89, 0.61, 0.003, 1.0)

void main()
{
	o_color = texture(color_texture_sampler, f_tex_coord);
	ivec3 dim = textureSize(color_grading_lut_texture_sampler, 0);
	if (dim.x > 1)
	{
		vec3 uvw = vec3(1.0 - o_color.g, o_color.r, o_color.b) + vec3(-0.5, 0.5, 0.5) / vec3(dim.y, dim.x, dim.z);
		o_color = texture(color_grading_lut_texture_sampler, uvw);
	}

	if (bool(pco.is_selecting) && texture(outline_texture_sampler, f_tex_coord).a > 0.5)
	{
	    o_color = OUTLINE_COLOR;
	}
}