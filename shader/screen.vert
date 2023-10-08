#version 450

layout (location = 0) out vec2 f_tex_coord;

void main()
{
	f_tex_coord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(f_tex_coord * 2.0f - 1.0f, 0.0f, 1.0f);
}