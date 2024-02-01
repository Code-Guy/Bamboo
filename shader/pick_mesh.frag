#version 450

layout(location = 0) in vec3 f_position;
layout(location = 1) in vec2 f_tex_coord;
layout(location = 2) in vec3 f_normal;

layout(location = 0) out vec4 o_color;

layout(push_constant) uniform PCO {
    vec4 color;
} pco;

void main()
{
	o_color = pco.color;
}