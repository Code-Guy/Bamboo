#version 450

layout(location = 0) in vec2 f_tex_coord;
layout(location = 0) out vec4 o_color;

layout(set = 0, binding = 0) uniform sampler2D bright_color_texture_sampler;

layout(push_constant) uniform PCO 
{
	int blur_direction;
} pco;

void main()
{
    float weight[5];
	weight[0] = 0.227027;
	weight[1] = 0.1945946;
	weight[2] = 0.1216216;
	weight[3] = 0.054054;
	weight[4] = 0.016216;

    vec2 tex_offset = 1.0 / textureSize(bright_color_texture_sampler, 0);
    vec3 result = texture(bright_color_texture_sampler, f_tex_coord).rgb * weight[0];

    if(pco.blur_direction == 1)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(bright_color_texture_sampler, f_tex_coord + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(bright_color_texture_sampler, f_tex_coord - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else if(pco.blur_direction == 0)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(bright_color_texture_sampler, f_tex_coord + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(bright_color_texture_sampler, f_tex_coord - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    o_color = vec4(result, 1.0);
}