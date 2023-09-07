#version 450

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

layout(push_constant) uniform PCO {
	layout(offset = 16) 
    vec2 size;
} pco;

layout(location = 0) out vec2 g_tex_coord;

void main()
{
	vec2 half_size = pco.size * 0.5;
	vec4 center = gl_in[0].gl_Position;

    gl_Position = vec4(center.xy + vec2(-half_size.x, -half_size.y), center.zw);
    g_tex_coord = vec2(0.0, 0.0);
    EmitVertex();

    gl_Position = vec4(center.xy + vec2(-half_size.x, half_size.y), center.zw);
    g_tex_coord = vec2(0.0, 1.0);
    EmitVertex();

    gl_Position = vec4(center.xy + vec2(half_size.x, -half_size.y), center.zw);
    g_tex_coord = vec2(1.0, 0.0);
    EmitVertex();

    gl_Position = vec4(center.xy + vec2(half_size.x, half_size.y), center.zw);
    g_tex_coord = vec2(1.0, 1.0);
    EmitVertex();

    EndPrimitive();
}