#version 450

layout(push_constant) uniform PCO { 
    vec4 position;
} pco;

void main()
{
	gl_Position = pco.position;
}