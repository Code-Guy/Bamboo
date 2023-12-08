#version 450

layout(location = 0) in vec2 f_tex_coord;
layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 0) uniform sampler2D color_texture_sampler;

layout(push_constant) uniform PCO 
{
	float threshold;
} pco;

void main()
{
    vec3 frag_color = texture(color_texture_sampler, f_tex_coord).rgb;
    float brightness = dot(frag_color, vec3(0.2126,0.7152,0.0722));
    if(brightness > pco.threshold)
        o_color = vec4(frag_color, 1.0);
    else
        o_color = vec4(0.0,0.0,0.0,1.0);
}