#version 450

layout(location = 0) out vec4 o_color;

layout(push_constant) uniform PCO {
    vec4 color;
} pco;

void main()
{
	o_color = color;
}