#version 450

layout(location = 0) in vec2 f_tex_coord;
layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 0) uniform sampler2D color_texture_sampler;
layout(set = 0, binding = 1) uniform sampler2D outline_texture_sampler;
layout(set = 0, binding = 2) uniform sampler3D color_grading_lut_texture_sampler;

void main()
{
	vec4 color = texture(color_texture_sampler, f_tex_coord);
	ivec3 dim = textureSize(color_grading_lut_texture_sampler, 0);
	vec3 uvw = vec3(1.0 - color.g, color.r, color.b) + vec3(-0.5, 0.5, 0.5) / vec3(dim.y, dim.x, dim.z);
    o_color = texture(color_grading_lut_texture_sampler, uvw);
}