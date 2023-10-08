#version 450

layout(location = 0) in vec2 g_tex_coord;

layout(binding = 0) uniform sampler2D texture_sampler;

layout(location = 0) out vec4 o_color;

void main()
{
	o_color = texture(texture_sampler, g_tex_coord);
}