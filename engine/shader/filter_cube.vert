#version 450

layout (location = 0) in vec3 position;

layout(push_constant) uniform PCO 
{
	layout (offset = 0) mat4 mvp;
} pco;

layout (location = 0) out vec3 f_uvw;

void main()
{
	f_uvw = position;
	gl_Position = pco.mvp * vec4(position.xyz, 1.0);
}
