#version 450 core
layout(location = 0) out vec4 fColor;

layout(set=0, binding=0) uniform sampler2D sTexture;

layout(location = 0) in struct {
    vec4 Color;
    vec2 UV;
} In;

void main()
{
    vec4 tColor = texture(sTexture, In.UV.st);
    if (length(In.Color.xyz) > 0)
    {
        fColor = In.Color * vec4(pow(tColor.xyz, vec3(1.0 / 2.2)), tColor.w);
    }
    else
    {
        fColor = tColor;
    }
}
