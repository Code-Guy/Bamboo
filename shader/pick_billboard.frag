#version 450

layout(location = 0) in vec2 g_tex_coord;

layout(location = 0) out vec4 o_color;

layout(push_constant) uniform PCO {
    layout(offset = 32) vec4 color;
} pco;

void main()
{
	o_color = pco.color;
}