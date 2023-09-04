#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(push_constant) uniform PCO { 
    mat4 mvp;
} pco;

layout(location = 0) out vec3 f_color;

void main()
{
	f_color = color;
	gl_Position = pco.mvp * vec4(position, 1.0);
}